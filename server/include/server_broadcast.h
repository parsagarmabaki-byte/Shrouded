#ifndef SERVER_BROADCAST_H
#define SERVER_BROADCAST_H

#include <SDL2/SDL_net.h>
#include "network_data.h"

int init_server_socket(UDPsocket *socket, TCPsocket *tcp_socket);
void cleanupServer(UDPsocket server_socket, TCPsocket tcp_socket, SDLNet_SocketSet socket_set, TCPsocket *tcp_sockets, UDPpacket *receive_packet, UDPpacket *send_packet);
void broadcast_msg(UDPsocket socket, UDPpacket *packet, IPaddress *clients, int *used, const void *data, size_t size);
void broadcast_game_state(UDPsocket socket, UDPpacket *packet, GameState *state, IPaddress *clients, int *used);
void broadcast_game_state_tcp(TCPsocket *sockets, GameState *state, int *used);
void broadcast_tcp_msg(TCPsocket *sockets, const void *data, size_t size);

#endif
