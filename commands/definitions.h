#pragma once
#include <dpp/dpp.h>
#include <vector>

inline std::vector<dpp::slashcommand> getGlobalCommands(const dpp::snowflake appId) {
    using namespace dpp;

    return {
        slashcommand("play", "plays a song", appId)
            .add_option(
                dpp::command_option(dpp::co_string, "query", "song to play", true)
            ),

        slashcommand("stop", "stops playing songs and clears the queue", appId),
    };
}