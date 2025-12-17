#pragma once
#include <dpp/dpp.h>
#include "../commands/deps/pbCtrl.h"

using namespace std;

using slashHandler = function<void(const dpp::slashcommand_t&)>;

inline void setupInteractionCreate(dpp::cluster& bot) {
    static unordered_map<string, slashHandler> commands;
    static music::PbCtrl pbCtrl(bot);

    // initialize once
    if (commands.empty()) {

        commands["stop"] = [&bot](const dpp::slashcommand_t& event) {
            pbCtrl.stop(event);
        };

        // capture bot reference for playCommand
        commands["play"] = [&bot](const dpp::slashcommand_t& event) {
            const std::string query = std::get<std::string>(event.get_parameter("query"));
            pbCtrl.handlePlay(event, query);
        };
    }

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
        if (const auto it = commands.find(event.command.get_command_name()); it != commands.end()) {
            it->second(event);
        }
    });
}