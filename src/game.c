#include "game.h"
#include <time.h>

Game* Game_new() {
    Game* game = (Game*)malloc(sizeof(Game));
    game->currentTurnMessage = NULL;
    game->objects = cJSON_CreateObject();
    game->width = 0;
    game->height = 0;
    return game;
}

cJSON* readAndParseJSON() {
    /*
        Reads one line of input from stdin and parses it as JSON.
        It will return the parsed JSON or if parsing failed, it will return NULL.
    */

    char input[5000];
    if (fgets(input, sizeof(input), stdin) == NULL) {
        perror("Error reading input from stdin");
        exit(EXIT_FAILURE);
    }

    cJSON* parsedInput = cJSON_Parse(input);
    if (parsedInput == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error parsing JSON: %s\n", error_ptr);
        }
    }
    return parsedInput;
}

void Game_init(Game* game) {
    /*
        The default constructor for the Game object.
        It will take care of the world init messages here and will save all map objects in the "objects" attribute.
        By the time the constructor is done and the object is created, the game has sent all init messages and the main
        communication cycle is about to begin.
    */

    // Read the first line to see our tank's id
    cJSON* firstMessage = readAndParseJSON();
    if (firstMessage == NULL) {
        fprintf(stderr, "Failed to read and parse the first message.\n");
        exit(EXIT_FAILURE);
    }

    cJSON* tankIdsMessage = cJSON_GetObjectItemCaseSensitive(firstMessage, "message");
    if (!cJSON_IsObject(tankIdsMessage)) {
        fprintf(stderr, "Error: Invalid JSON format - missing 'message' field in the first message.\n");
        exit(EXIT_FAILURE);
    }

    cJSON* tankIdJson = cJSON_GetObjectItemCaseSensitive(tankIdsMessage, "your-tank-id");
    if (!cJSON_IsString(tankIdJson)) {
        fprintf(stderr, "Error: Invalid JSON format - 'your-tank-id' field should be a string.\n");
        exit(EXIT_FAILURE);
    }

    strcpy(game->tankId, tankIdJson->valuestring);

    cJSON* enemyTankIdJson = cJSON_GetObjectItemCaseSensitive(tankIdsMessage, "enemy-tank-id");
    if (!cJSON_IsString(enemyTankIdJson)) {
        fprintf(stderr, "Error: Invalid JSON format - 'enemy-tank-id' field should be a string.\n");
        exit(EXIT_FAILURE);
    }

    strcpy(game->enemyTankId, enemyTankIdJson->valuestring);

    // Now start reading the map info until the END_INIT signal is received.
    cJSON* nextMessage = readAndParseJSON();
    while (!cJSON_IsString(nextMessage) || strcmp(nextMessage->valuestring, "END_INIT") != 0) {
        // Get the updated_objects part
        cJSON* updatedObjects = cJSON_GetObjectItemCaseSensitive(nextMessage, "message");
        if (!cJSON_IsObject(updatedObjects)) {
            fprintf(stderr, "Error: Invalid JSON format - missing 'message' field in an update message.\n");
            exit(EXIT_FAILURE);
        }
        updatedObjects = cJSON_GetObjectItemCaseSensitive(updatedObjects, "updated_objects");
        if (!cJSON_IsObject(updatedObjects)) {
            fprintf(stderr, "Error: Invalid JSON format - missing 'updated_objects' field in an update message.\n");
            exit(EXIT_FAILURE);
        }

        // Loop through all passed objects and save them
        cJSON* object;
        cJSON_ArrayForEach(object, updatedObjects) {
            cJSON_AddItemReferenceToObject(game->objects, object->string, cJSON_Duplicate(object, 1));
        }

        // Read the next message
        cJSON_Delete(nextMessage);
        nextMessage = readAndParseJSON();
    }

    // We are outside the loop now, which means END_INIT signal was received.

    // Let's figure out the size of the map based on the given boundaries.
    // Set width and height = 0 then find the maximum x and y - they have to be the width and height of the map.
    game->width = 0;
    game->height = 0;
    cJSON* object_it;
    cJSON_ArrayForEach(object_it, game->objects) {
        // cJSON* object = cJSON_GetObjectItemCaseSensitive(game->objects, object_it->string);
        cJSON* typeJson = cJSON_GetObjectItemCaseSensitive(object_it, "type");
        if (!cJSON_IsNumber(typeJson)) {
            fprintf(stderr, "Error: Invalid JSON format - 'type' field should be a number.\n");
            exit(EXIT_FAILURE);
        }

        int type = typeJson->valueint;
        if (type == BOUNDARY) {
            cJSON* position = cJSON_GetObjectItemCaseSensitive(object_it, "position");
            if (!cJSON_IsArray(position) || cJSON_GetArraySize(position) != 4) {
                fprintf(stderr, "Error: Invalid JSON format - 'position' field should be an array of size 4.\n");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < 4; i++) {
                cJSON* pos = cJSON_GetArrayItem(position, i);
                if (!cJSON_IsArray(pos) || cJSON_GetArraySize(pos) != 2) {
                    fprintf(stderr, "Error: Invalid JSON format - 'position' subarray should be of size 2.\n");
                    exit(EXIT_FAILURE);
                }
                double x = cJSON_GetArrayItem(pos, 0)->valuedouble;
                double y = cJSON_GetArrayItem(pos, 1)->valuedouble;

                game->width = (x > game->width) ? x : game->width;
                game->height = (y > game->height) ? y : game->height;
            }
        }
    }
}

int readNextTurnData(Game* game) {
    /*
        This function reads the data for this turn and returns 1 if the game continues (i.e. END signal is not received) otherwise
        it will return 0.
    */

    if (game->currentTurnMessage != NULL) {
        cJSON_Delete(game->currentTurnMessage);
        game->currentTurnMessage = NULL;
    }

    game->currentTurnMessage = readAndParseJSON();
    if (game->currentTurnMessage == NULL) {
        return 0; // End the game if there's an error reading and parsing JSON.
    }

    // Return 0 if the game is over
    if (cJSON_IsString(game->currentTurnMessage) && strcmp(game->currentTurnMessage->valuestring, "END") == 0) {
        cJSON_Delete(game->currentTurnMessage);
        game->currentTurnMessage = NULL;
        return 0;
    }

    // First delete the objects that have been deleted.
    cJSON* deletedObjects = cJSON_GetObjectItemCaseSensitive(game->currentTurnMessage, "message");
    if (cJSON_IsObject(deletedObjects)) {
        deletedObjects = cJSON_GetObjectItemCaseSensitive(deletedObjects, "deleted_objects");
        if (cJSON_IsArray(deletedObjects)) {
            cJSON* deleted_id;
            cJSON_ArrayForEach(deleted_id, deletedObjects) {
                cJSON_DeleteItemFromObject(game->objects, deleted_id->valuestring);
            }
        }
    }

    // Now add all the new/updated objects
    cJSON* updatedObjects = cJSON_GetObjectItemCaseSensitive(game->currentTurnMessage, "message");
    if (cJSON_IsObject(updatedObjects)) {
        updatedObjects = cJSON_GetObjectItemCaseSensitive(updatedObjects, "updated_objects");
        if (cJSON_IsObject(updatedObjects)) {
            cJSON* object_it = NULL;
            cJSON* object_item = updatedObjects->child;
            while (object_item != NULL) {
                cJSON* object = cJSON_GetObjectItemCaseSensitive(updatedObjects, object_item->string);
                cJSON_AddItemReferenceToObject(game->objects, object_item->string, cJSON_Duplicate(object, 1));
                object_item = object_item->next;
            }
        }
    }

    // Game continues!
    return 1;
}

void respondToTurn(Game* game) {
    /*
        Your logic goes here. Respond to the turn with an action. You can always assume readNextTurnData is called before this.
    */

    // Write your code here...

    // For demonstration, let's always shoot at a random angle for now.
    cJSON* response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "shoot", rand() % 360);
    char* responseStr = cJSON_PrintUnformatted(response);
    if (responseStr != NULL) {
        puts(responseStr);
        free(responseStr);
    }
    cJSON_Delete(response);
}
