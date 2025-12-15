#pragma once
#include <dpp/dpp.h>

inline void setupReady(dpp::cluster& bot) {
    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            bot.global_command_create(
                dpp::slashcommand("ping", "ping pong!", bot.me.id)
            );
        }
    });
}