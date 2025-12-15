#pragma once
#include <dpp/dpp.h>
#include <vector>

inline std::vector<dpp::slashcommand> getGlobalCommands(const dpp::snowflake appId) {
    using namespace dpp;

    return {
        slashcommand("ping", "ping pong!", appId)
    };
}