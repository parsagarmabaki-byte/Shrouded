#ifndef SERVER_GAME_LOGIC_H
#define SERVER_GAME_LOGIC_H

#include "network_data.h"

void check_win_condition(gameState *state);
void apply_player_input(gameState *state, clientInput *input, float dt);

#endif
