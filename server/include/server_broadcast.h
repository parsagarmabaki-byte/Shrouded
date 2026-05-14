#ifndef SERVER_BROADCAST_H
#define SERVER_BROADCAST_H

#include <SDL2/SDL_net.h>
#include "network_data.h"

int init_server_socket(UDPsocket *socket);
void cleanupServer(UDPsocket server_socket, UDPpacket *receive_packet, UDPpacket *send_packet);
void broadcastGameState(UDPsocket socket, UDPpacket *packet, gameState *state, IPaddress *clientAddresses, int *clientUsed);
void broadcast_Kill_msg(UDPsocket socket, UDPpacket *packet, KillEventMsg *msg, IPaddress *clientAddresses, int *clientUsed);
void broadcast_emergency_meeting_msg(UDPsocket socket, UDPpacket *packet, KillEventMsg *msg, IPaddress *clientAddresses, int *clientUsed);

#endif
