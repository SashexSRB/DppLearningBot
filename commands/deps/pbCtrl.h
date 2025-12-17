#pragma once

#include "songRepo.h"
#include <dpp/dpp.h>
#include <filesystem>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <cstdio>
#include <regex>
#include <ctime>
#include <vector>

namespace fs = std::filesystem;

namespace music {

class PbCtrl {
public:
    explicit PbCtrl(dpp::cluster& bot)
        : bot(bot), songRepo(fs::current_path() / "songs")
    {
        bot.on_voice_ready([this](const dpp::voice_ready_t& event) {
            auto& state = getState(event.voice_client->server_id);
            state.voiceClient = event.voice_client;
            if (!state.isPlaying && !state.queue.empty()) {
                playNext(event.voice_client->server_id);
            }
        });
    }

    void handlePlay(const dpp::slashcommand_t& event, const std::string& query) {
        const dpp::snowflake guildId = event.command.guild_id;
        auto& state = getState(guildId);

        dpp::guild* guild = dpp::find_guild(guildId);
        if (!guild) {
            event.reply(dpp::message("Guild not cached").set_flags(dpp::m_ephemeral));
            return;
        }

        auto voiceIt = guild->voice_members.find(event.command.usr.id);
        if (voiceIt == guild->voice_members.end() || voiceIt->second.channel_id == 0) {
            event.reply(dpp::message("You must be in a voice channel").set_flags(dpp::m_ephemeral));
            return;
        }

        event.thinking(true);

        if (!state.voiceClient) {
            if (!guild->connect_member_voice(bot, event.command.usr.id, false, true, false)) {
                event.edit_original_response(dpp::message("You must be in a voice channel."));
                return;
            }
            event.edit_original_response(dpp::message("Joining voice channel..."));
        }

        std::string title;
        std::string url;
        try {
            std::tie(title, url) = fetchTitleAndUrl(query);
        } catch (...) {
            event.edit_original_response(dpp::message("Could not find a video for your query."));
            return;
        }

        SongData song;
        const SongData* cached = songRepo.getCachedSong(query, url);
        if (cached && fs::exists(cached->filePath)) {
            song = *cached;
            event.edit_original_response(dpp::message("♻️ Using cached: **" + song.title + "**"));
        } else {
            song.title = title;
            song.url = url;
            song.filePath = songRepo.getSongsDir() / (title + ".mp3");
            downloadSong(event, song);
            songRepo.storeSong(query, song);
        }

        {
            std::lock_guard<std::mutex> lock(state.mutex);
            state.queue.push_back(song);
        }

        size_t position;
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            position = state.queue.size();
        }

        if (position > 1) {
            bot.message_create(dpp::message(event.command.channel_id,
                "✅ Queued: **" + song.title + "** (position " + std::to_string(position) + ")"));
        }

        if (!state.isPlaying) {
            playNext(guildId);
        }
    }

    void skip(const dpp::slashcommand_t& event) {
        auto& state = getState(event.command.guild_id);
        if (state.isPlaying && state.voiceClient) {
            state.stopRequested = true;
            event.reply("⏭ Skipped.");
        } else {
            event.reply("Nothing playing.");
        }
    }

    void stop(const dpp::slashcommand_t& event) {
        auto& state = getState(event.command.guild_id);
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            state.queue.clear();
        }
        state.stopRequested = true;
        if (state.voiceClient) {
            bot.current_user_set_voice_state(event.command.guild_id, 0, false, false);
            state.voiceClient = nullptr;
        }
        state.isPlaying = false;
        event.reply("⏹ Stopped playback.");
    }

    void showQueue(const dpp::slashcommand_t& event) {
        auto& state = getState(event.command.guild_id);
        std::lock_guard<std::mutex> lock(state.mutex);
        if (state.queue.empty()) {
            event.reply("Queue is empty.");
            return;
        }
        std::string msg = "🎵 **Queue:**\n";
        size_t i = 1;
        for (const auto& s : state.queue) {
            msg += std::to_string(i++) + ". " + s.title + "\n";
            if (msg.size() > 1800) break;
        }
        event.reply(msg);
    }

private:
    struct GuildState {
        std::deque<SongData> queue;
        dpp::discord_voice_client* voiceClient{nullptr};
        std::atomic<bool> isPlaying{false};
        std::atomic<bool> stopRequested{false};
        std::mutex mutex;
    };


    dpp::cluster& bot;
    SongRepo songRepo;
    std::unordered_map<dpp::snowflake, GuildState> guildStates;

    GuildState& getState(dpp::snowflake guildId) {
        return guildStates[guildId];
    }

    static std::pair<std::string, std::string> fetchTitleAndUrl(const std::string& query) {
        std::string search = query.starts_with("http") ? query : "ytsearch:" + query;
        std::string cmd = "yt-dlp --dump-json --no-playlist \"" + search + "\"";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) throw std::runtime_error("yt-dlp failed");
        std::string out;
        char buf[1024];
        while (fgets(buf, sizeof(buf), pipe)) out += buf;
        pclose(pipe);
        auto j = json::parse(out);
        if (j.contains("entries")) j = j["entries"][0];
        std::string url = j.value("webpage_url", "");
        if (url.empty()) throw std::runtime_error("No URL");
        std::string title = j.value("title", "track-" + std::to_string(std::time(nullptr)));
        title = std::regex_replace(title, std::regex(R"([<>:\"/\\|?*\x00-\x1F])"), "_");
        return {title, url};
    }

    static void downloadSong(const dpp::slashcommand_t& event, const SongData& song) {
        std::string cmd = "yt-dlp -x --audio-format mp3 -o \"" + song.filePath.string() + "\" \"" + song.url + "\"";
        system(cmd.c_str());
        event.edit_original_response(dpp::message("⬇️ Downloaded: **" + song.title + "**"));
    }

    void playNext(dpp::snowflake guildId) {
        auto& state = getState(guildId);
        if (!state.voiceClient || state.isPlaying.load()) {
            return;
        }

        SongData song;
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            if (state.queue.empty()) {
                state.isPlaying = false;
                return;
            }
            song = state.queue.front();
            state.queue.pop_front();
            state.isPlaying = true;
            state.stopRequested = false;
        }

        std::thread([this, guildId, song = std::move(song)]() mutable {
            auto& state = getState(guildId);

            /* ---------- mpg123 setup ---------- */
            int err = MPG123_OK;
            mpg123_handle* mh = mpg123_new(nullptr, &err);
            if (!mh || err != MPG123_OK) {
                state.isPlaying = false;
                playNext(guildId);
                return;
            }

            /* Force EXACT Discord format */
            mpg123_format_none(mh);
            mpg123_format(mh, 48000, 2, MPG123_ENC_SIGNED_16);

            if (mpg123_open(mh, song.filePath.string().c_str()) != MPG123_OK) {
                mpg123_delete(mh);
                state.isPlaying = false;
                playNext(guildId);
                return;
            }

            size_t buffer_size = mpg123_outblock(mh);
            std::vector<uint8_t> buffer(buffer_size);

            /* ---------- decode + stream ---------- */
            size_t done = 0;
            while (!state.stopRequested.load() &&
                   mpg123_read(mh, buffer.data(), buffer_size, &done) == MPG123_OK) {

                if (done == 0)
                    continue;

                /* Send PCM as samples (not bytes) */
                state.voiceClient->send_audio_raw(
                    reinterpret_cast<uint16_t*>(buffer.data()),
                    done / sizeof(uint16_t)
                );

                /* Discord requires ~20ms pacing */
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }

            mpg123_close(mh);
            mpg123_delete(mh);

            state.isPlaying = false;
            playNext(guildId);
        }).detach();
    }

};

} // namespace music
