#pragma once
#include <dpp/dpp.h>

inline void setupReady(dpp::cluster& bot) {
    bot.on_ready([&bot](const dpp::ready_t& event) {
        std::cout << "Bot online!\n";
    });
}