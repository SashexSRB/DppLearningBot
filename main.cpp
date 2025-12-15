#include <dpp/dpp.h>
#include "events/interactionCreate.h"
#include "events/ready.h"
#include "includes/dotenv.h"

using namespace std;

int main() {
    if (!dotenv::config.load("../.env")) {
        throw std::runtime_error("Failed to load .env file\n");
    }

    const string BOT_TOKEN = dotenv::config.get<string>("TOKEN", "0");

    dpp::cluster bot(BOT_TOKEN);

    bot.on_log(dpp::utility::cout_logger());

    setupInteractionCreate(bot);
    setupReady(bot);

    bot.start(dpp::st_wait);

    return 0;
}