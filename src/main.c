#include "game.h"
#include <time.h>

int main() {
    srand(time(0));

    Game* game = Game_new();
    Game_init(game);

    while (readNextTurnData(game))
        respondToTurn(game);

    cJSON_Delete(game->currentTurnMessage);
    cJSON_Delete(game->objects);
    free(game);

    return 0;
}