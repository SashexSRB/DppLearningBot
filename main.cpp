#include <dpp/dpp.h>
#include <mpg123.h>
#include "events/events.h"
#include "includes/dotenv.h"

using namespace std;

int main() {
    if (!dotenv::config.load("../.env")) {
        throw std::runtime_error("Failed to load .env file\n");
    }

    const auto BOT_TOKEN = dotenv::config.get<string>("TOKEN", "0");

    mpg123_init();

    dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_guild_voice_states);

    bot.on_log(dpp::utility::cout_logger());

    setupInteractionCreate(bot);
    setupReady(bot);

    bot.start(dpp::st_wait);

    mpg123_exit();

    return 0;
}