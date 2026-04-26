#ifndef NETWORK_H
#define NETWORK_H

#include <SDL2/SDL_net.h>

int init_network_socket(UDPsocket *socket, Uint16 port);
UDPpacket *create_packet(int size);
int send_packet_data(UDPsocket socket, UDPpacket *packet, IPaddress address, const void *data, int size);
int packet_has_size(UDPpacket *packet, int expectedsize, const char *label);

#endif
