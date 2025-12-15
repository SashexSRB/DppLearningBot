#include <dpp/dpp.h>
#include <iostream>
#include "includes/dotenv.h"
#include "commands/definitions.h"
#include <string>

using namespace std;

int main() {
    if (!dotenv::config.load("../.env")) {
        throw std::runtime_error("Failed to load .env file\n");
    }

    const auto BOT_TOKEN = dotenv::config.get<string>("TOKEN", "0");

    dpp::cluster bot(BOT_TOKEN);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_ready([&bot](const dpp::ready_t& event) {
        std::cout << "Registering commands...\n";

        const auto commands = getGlobalCommands(bot.me.id);

        bot.global_bulk_command_create(
            commands
        );

        std::cout << "Done.\n";
        bot.shutdown();
    });

    bot.start(dpp::st_wait);
    return 0;
}
