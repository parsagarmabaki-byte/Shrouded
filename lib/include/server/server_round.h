#ifndef SERVER_ROUND_H
#define SERVER_ROUND_H

#include <SDL2/SDL.h>
#include "network_data.h"

int designateImpostor(gameState *state);
void start_new_round(gameState *state, Uint64 *state_start_time);

#endif
