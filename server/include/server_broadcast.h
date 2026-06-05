#ifndef SERVER_BROADCAST_H
#define SERVER_BROADCAST_H

#include <SDL2/SDL_net.h>
#include "network_data.h"

int init_server_socket(UDPsocket *socket);
void cleanupServer(UDPsocket server_socket, UDPpacket *receive_packet, UDPpacket *send_packet);
void broadcast_msg(UDPsocket socket, UDPpacket *packet, IPaddress *clients, int *used, const void *data, size_t size);
void broadcast_game_state(UDPsocket socket, UDPpacket *packet, gameState *state, IPaddress *clients, int *used);

#endif
