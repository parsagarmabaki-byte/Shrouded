#include <stdio.h>
#include <string.h>
#include <SDL2/SDL_net.h>
#include "network.h"

int init_network_socket(UDPsocket *socket, Uint16 port)
{
    if (SDLNet_Init() != 0)
    {
        printf("SDLNet_Init error: %s\n", SDLNet_GetError());
        return 0;
    }

    *socket = SDLNet_UDP_Open(port);
    if (!*socket)
    {
        printf("SDLNet_UDP_Open error: %s\n", SDLNet_GetError());
        return 0;
    }

    return 1;
}

UDPpacket *create_packet(int size)
{
    UDPpacket *packet = SDLNet_AllocPacket(size);
    if (!packet)
    {
        printf("Failed to allocate packet: %s\n", SDLNet_GetError());
    }

    return packet;
}

int send_packet_data(UDPsocket socket, UDPpacket *packet, IPaddress address, const void *data, int size)
{
    memcpy(packet->data, data, size);
    packet->len = size;
    packet->address = address;

    if (!SDLNet_UDP_Send(socket, -1, packet))
    {
        printf("SDLNet_UDP_Send error: %s\n", SDLNet_GetError());
        return 0;
    }

    return 1;
}
