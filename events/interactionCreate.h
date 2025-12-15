#pragma once
#include <dpp/dpp.h>
#include "../commands/ping.h"

using namespace std;

using slashHandler = function<void(const dpp::slashcommand_t&)>;

inline void setupInteractionCreate(dpp::cluster& bot) {
    static const unordered_map<string, slashHandler> commands = {
        {"ping", pingCommand},
    };

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
        if (const auto it = commands.find(event.command.get_command_name()); it != commands.end()) {
            it->second(event);
        }
    });
}