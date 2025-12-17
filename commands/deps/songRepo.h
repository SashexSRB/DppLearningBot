#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <dpp/nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace music {
    struct SongData {
        std::string title;
        std::string url;
        fs::path filePath;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(SongData, title, url, filePath);
    };

    class SongRepo {
    public:
        explicit SongRepo(const fs::path& rootDir = fs::current_path() / "songs") {
            songsDir = rootDir;
            cacheFile = songsDir / "songRepo.json";
            ensureDirectoriesAndFile();
            loadCache();
        }

        [[nodiscard]] const SongData* getCachedSong(std::string query, std::string url = {}) const {
            trim(query);
            const std::string queryLower = toLower(query);
            auto queryWords = tokenizeAndFilter(queryLower);

            // 1. Exact URL match, highest priority
            if (!url.empty()) {
                trim(url);
                for (const auto& [_, song] : cache) {
                    if (!song.url.empty() && song.url == url) return &song;
                }
            }

            // 2. Fuzzy title matching
            const SongData* bestMatch = nullptr;
            double bestScore = 0.0;

            for (const auto& [_, song] : cache) {
                std::string titleLower = toLower(song.title);
                trim(titleLower);
                auto titleWords = tokenizeAndFilter(titleLower);

                double score = jaccardSimilarity(queryWords, titleWords);

                // Big boost: full query contained in title
                if (titleLower.find(queryLower) != std::string::npos) {
                    score += 0.5;
                }
                // Medium boost: clean title (without training remix/dash part) inside query
                else {
                    std::string cleanTitle = titleLower;
                    size_t dashPos = cleanTitle.find("-–—");
                    if (dashPos != std::string::npos) {
                        cleanTitle = cleanTitle.substr(0, dashPos);
                    }
                    trim(cleanTitle);

                    if (!cleanTitle.empty() && queryLower.find(cleanTitle) != std::string::npos) {
                        score += 0.3;
                    }
                    // Small boost: any of the first 3 title words appear in query
                    else if (!queryWords.empty() && !titleWords.empty()) {
                        const size_t check = std::min<size_t>(3, titleWords.empty());
                        for (size_t i = 0; i < check; ++i) {
                            if (std::find(queryWords.begin(), queryWords.end(), titleWords[i]) != queryWords.end()) {
                                score += 0.1;
                                break;
                            }
                        }
                    }
                }

                if (score > bestScore) {
                    bestScore = score;
                    bestMatch = &song;
                }
            }
            return (bestScore >= 0.55) ? bestMatch : nullptr;
        }

        void storeSong(std::string query, const SongData& data) const {
            trim(query);
            const std::string normalizedQuery = toLower(query);

            // Deduplicate by URL first
            if (!data.url.empty()) {
                std::string cleanUrl = data.url;
                trim(cleanUrl);
                for (auto& [_, song] : cache) {
                    if (!song.url.empty() && song.url == cleanUrl) {
                        song = data;
                        saveCache();
                        return;
                    }
                }
            }

            cache[normalizedQuery] = data;
            saveCache();
        }

        [[nodiscard]] fs::path getSongsDir() const { return songsDir;}

    private:
        fs::path songsDir;
        fs::path cacheFile;
        mutable std::unordered_map<std::string, SongData> cache;

        static constexpr std::array noiseWords = {
            "the", "and", "official", "audio", "video", "lyrics"
        };

        void ensureDirectoriesAndFile() const {
            if (!fs::exists(songsDir)) {
                fs::create_directories(songsDir);
            }
            if (!fs::exists(cacheFile)) {
                std::ofstream{cacheFile} << "{}";
            }
        }

        void loadCache() const {
            try {
                std::ifstream ifs(cacheFile);
                if (!ifs.is_open()) {
                    std::cerr << "Warning: Could not open cache file " << cacheFile << "\n";
                    return;
                }

                json j;
                ifs >> j;

                for (auto& [key, value] : j.items()) {
                    cache[key] = value.get<SongData>();
                }
            } catch (const std::exception& e) {
                std::cerr << "Failed to load song cache, starting fresh: " << e.what() << "\n";
            }
        }

        void saveCache() const {
            try {
                json j;
                for (const auto& [key, data] : cache) {
                    j[key] = data;
                }
                std::ofstream ofs(cacheFile);
                ofs << std::setw(4) << j << '\n';
            } catch (const std::exception& e) {
                std::cerr << "Failed to save song cache: " << e.what() << "\n";
            }
        }

        static std::vector<std::string> tokenizeAndFilter(const std::string& text) {
            std::vector<std::string> words;
            std::istringstream iss(text);
            std::string word;

            while (iss >> word) {
                word = toLower(word);
                if (word.length() <= 2) continue;
                if (std::find(noiseWords.begin(), noiseWords.end(), word) != noiseWords.end()) continue;
                words.push_back(std::move(word));
            }
            return words;
        }

        static double jaccardSimilarity(const std::vector<std::string>& a, const std::vector<std::string>& b) {
            if (a.empty() && b.empty()) return 0.0;

            const std::unordered_set<std::string> setA(a.begin(), a.end());
            const std::unordered_set<std::string> setB(b.begin(), b.end());

            size_t intersection = 0;
            for (const auto& w : setA) {
                if (setB.contains(w)) ++intersection;
            }

            const size_t unionSize = setA.size() + setB.size() - intersection;
            return unionSize == 0 ? 0.0 : static_cast<double>(intersection) / static_cast<double>(unionSize);
        }

        static std::string toLower(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), [](const unsigned char c){return tolower(c);});
            return s;
        }

        static void trim(std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
        }
    };
} // namespace music
