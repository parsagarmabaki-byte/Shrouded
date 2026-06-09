#ifndef GAME_UPDATE_H
#define GAME_UPDATE_H

#include <SDL2/SDL.h>
#include "network_data.h"
#include "task.h"
#include "player_movement.h"

typedef struct GameContext GameContext;
typedef struct Client Client;

void update_game(GameContext *ctx);

float calculate_delta_time(Uint64 *last_tick);

void update_player_direction(Player *player, InputMsg *user_input);
void update_player_movement(Player *player, InputMsg *user_input, bool task_is_active, bool emergency_window_open, float *accumulator);

void send_player_input(Client *client, Player *player, InputMsg input, bool task_is_active, bool emergency_window_open);
void update_task_check_completion(Client *client, Task *task, GameState *state, int local_id, float dt, bool *was_task_active);

#endif