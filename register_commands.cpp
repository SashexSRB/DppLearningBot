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

        for (const auto& cmd : commands) {
           std::cout << "Registering command: " << cmd.name << "\n";

           bot.global_command_create(cmd, [&bot, &cmd](const dpp::confirmation_callback_t& callback) {
               if (callback.is_error()) {
                   std::cerr << "Failed to register command '" << cmd.name
                             << "': " << callback.get_error().message << "\n";
               } else {
                   std::cout << "Successfully registered command '" << cmd.name << "'\n";
               }

           });
       }
        std::cout << "All registration requests sent. Keep the bot running.\n";
    });

    bot.start(dpp::st_wait);
    return 0;
}
