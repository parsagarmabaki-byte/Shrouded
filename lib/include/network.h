#ifndef NETWORK_H
#define NETWORK_H

#include <SDL2/SDL_net.h>
#include "network_data.h"

int init_server(UDPsocket *socket);
int init_client(UDPsocket *socket, IPaddress *server_addr);

int send_join(UDPsocket socket, IPaddress server_addr);
int send_start_game(UDPsocket socket, IPaddress server_addr);
int send_client_input(UDPsocket socket, IPaddress server_addr, clientInput *input);

int receive_client_input(UDPsocket socket, UDPpacket *packet, clientInput *input);
int receive_game_state(UDPsocket socket, UDPpacket *packet, gameState *state);

int send_game_state(UDPsocket socket, UDPpacket *packet, IPaddress addr, gameState *state);
int send_leave(UDPsocket socket, IPaddress server_addr);


#endif