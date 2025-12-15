#pragma once

#include <fstream>
#include <string>
#include <unordered_map>
#include <optional>
#include <sstream>
#include <cstdlib>
#include <algorithm>

namespace dotenv {

class Config {
    std::unordered_map<std::string, std::string> vars;

    static inline std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }

public:
    // Load the .env file, parse and store variables internally,
    // also sets OS environment variables for compatibility
    bool load(const std::string& path = ".env") {
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            auto pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));

            // Remove surrounding quotes (only if both ends are quotes)
            if (!value.empty() && value.front() == '"' && value.back() == '"')
                value = value.substr(1, value.size() - 2);

            vars[key] = value;

            // Set OS environment variable
            #ifdef _WIN32
                _putenv_s(key.c_str(), value.c_str());
            #else
                setenv(key.c_str(), value.c_str(), 1);
            #endif
        }
        return true;
    }

    // Retrieve raw string value if it exists
    std::optional<std::string> get(const std::string& key) const {
        auto it = vars.find(key);
        if (it != vars.end()) return it->second;
        return std::nullopt;
    }

    // Retrieve typed value with default fallback
    template<typename T>
    T get(const std::string& key, T defaultValue) const {
        auto opt = get(key);
        if (!opt) return defaultValue;

        // Specialize bool parsing
        if constexpr(std::is_same_v<T, bool>) {
            std::string valLower = *opt;
            std::transform(valLower.begin(), valLower.end(), valLower.begin(), ::tolower);
            if (valLower == "true" || valLower == "1") return true;
            if (valLower == "false" || valLower == "0") return false;
            return defaultValue;
        }

        // Parse numeric or string types
        std::istringstream iss(*opt);
        T value;
        iss >> value;
        if (iss.fail()) return defaultValue;
        return value;
    }
};

inline Config config; // singleton instance

} // namespace dotenv