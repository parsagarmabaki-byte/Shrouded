#ifndef CLIENT_NETWORK_H
#define CLIENT_NETWORK_H

#include <stddef.h>
#include <SDL2/SDL_net.h>
#include "game.h"
#include "kill_animation.h"

int init_client(Client *client, const char *server_ip, char *error_message, size_t error_size);
void clean_client(Client *client);
int send_join(Client *client);
int send_start_game(Client *client);
void send_input(Client *client, gameState *state, Player *player);
void request_kill(Client *client, gameState *state);
void collect_packets(Client *client, gameState *state, KillAnimation *bodies);
int send_leave_message(Client *client);
void request_emergency_meeting(Client *client, gameState *state, int local_id);

#endif
