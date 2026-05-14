#ifndef GAME_INPUT_H
#define GAME_INPUT_H

#include <stdbool.h>
#include "network_data.h"

typedef struct GameContext GameContext;

void process_events(GameContext *ctx);
clientInput read_input(bool tasks_active);

#endif