#pragma once
#include <dpp/dpp.h>

inline void pingCommand(const dpp::slashcommand_t& event) {
    event.reply("pong!");
}