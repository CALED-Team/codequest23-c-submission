# CodeQuest 23 C Submission Template

Use this repo as starting point for your C++ submissions to CodeQuest 23.

## Running the code

To compile and test on your local, either compile with gcc:

```
gcc -o src/game.out src/main.c src/game.c src/cjson/cJSON.c
```

and run:

```
./game.out
```

Or run using the docker image (which would ensure you get the same output as on CodeQuest servers) from the root directory:

```
docker build . -t my-submission:latest
docker run -it my-submission:latest
```

## Working on your bot

All functions in the code are commented so I would recommend checking the comments first.

Most of the logic of the game is inside `game.c` file. The input is already read and parsed for you. You can, if you need, change the
input reading logic.

The main part of the code you should change is `respondToTurn` function. Use the `game->objects` (and `game->currentTurnMessage` if you need) attributes to process what's happening in the game and respond with an action.