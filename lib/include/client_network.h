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
int send_task_complete(Client *client, int player_id, TaskType task_type);
void request_kill(Client *client, gameState *state);
void collect_packets(Client *client, gameState *state, KillAnimation *bodies);
int send_leave_message(Client *client);
void request_emergency_meeting(Client *client, gameState *state, int local_id);
void request_report_body(Client *client, gameState *state, KillAnimation dead_body, int target_id);
static int send_client_vote_packet(UDPsocket socket, IPaddress server_addr, VoteRequest *vote);
void send_vote(Client *client, int targeted_banner);

#endif
