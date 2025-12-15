#include <dpp/dpp.h>
#include "events/interactionCreate.h"
#include "events/ready.h"

using namespace dpp;
using namespace std;

const string BOT_TOKEN = "MTQ1MDEwNDczODUwNTgxODIwMw.GejPux.9xoOw7xAx_4pUPGH2TH47rFyfsOXWk6K9GXaec";

int main() {
    cluster bot(BOT_TOKEN);

    bot.on_log(utility::cout_logger());

    setupInteractionCreate(bot);
    setupReady(bot);

    bot.start(st_wait);

    return 0;
}