#ifndef CLIENT_NETWORK_H
#define CLIENT_NETWORK_H

#include <SDL2/SDL_net.h>
#include "game.h"

void send_input(Client *client, gameState *state, Player *player);
void request_kill(Client *client, gameState *state);
void collect_packets(Client *client, gameState *state, KillAnimation *bodies);
int send_leave_message(UDPsocket socket, IPaddress server_addr);

#endif
