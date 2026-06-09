#ifndef SERVER_ROUND_H
#define SERVER_ROUND_H

#include <SDL2/SDL.h>
#include "network_data.h"

int designate_killer(GameState *state);
void start_new_round(GameState *state, Uint64 *state_start_time, int *killer_id);

#endif
