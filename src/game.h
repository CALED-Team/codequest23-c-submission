#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cjson/cJSON.h"
#include "types.h"

typedef struct {
    char tankId[100];           // Your client's tank id
    cJSON* currentTurnMessage;  // The last received message (updated in readNextTurnData)
    cJSON* objects;             // All the objects with the same format as described in the doc. The key will be object id and value will be the object itself.
    double width;               // The width of the map.
    double height;              // The height of the map.
} Game;

Game* Game_new();
cJSON* readAndParseJSON();
void Game_init(Game* game);
int readNextTurnData(Game* game);
void respondToTurn(Game* game);

#endif  // GAME_H
