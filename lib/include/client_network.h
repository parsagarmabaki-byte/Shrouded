#ifndef CLIENT_NETWORK_H
#define CLIENT_NETWORK_H

#include <stddef.h>
#include <SDL2/SDL_net.h>
#include "game.h"
#include "kill_animation.h"
#include "SFX.h"

int init_client(Client *client, const char *server_ip, char *error_message, size_t error_size);
void clean_client(Client *client);
int send_join(Client *client);
int send_start_game(Client *client);
int send_play_again(Client *client);
void send_input(Client *client, Player *player, InputMsg input_msg);
int send_packet(UDPsocket socket, IPaddress server_addr, const void *data, size_t size);
int send_task_complete(Client *client, int player_id, TaskType task_type);
void request_kill(Client *client, int target_id);
void collect_packets(Client *client, gameState *state, KillAnimation *bodies, AudioAssets *audio);
int send_leave_message(Client *client);
void request_emergency_meeting(Client *client, gameState *state, int local_id);
void request_report_body(Client *client, int body_id);
void send_vote(Client *client, int targeted_banner, int voter_id);
int send_debug_win(Client *client, MessageType type);

#endif
