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

int init_tcp_server_socket(TCPsocket *socket, Uint16 port)
{
    IPaddress ip;
    if (SDLNet_ResolveHost(&ip, NULL, port) != 0)
    {
        printf("SDLNet_ResolveHost TCP server error: %s\n", SDLNet_GetError());
        return 0;
    }

    *socket = SDLNet_TCP_Open(&ip);
    if (!*socket)
    {
        printf("SDLNet_TCP_Open server error: %s\n", SDLNet_GetError());
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
    if (packet->maxlen < size)
    {
        printf("ERROR: Send packet too small/big\n");
        return 0;
    }
    else
    {
        memcpy(packet->data, data, size);
        packet->len = size;
        packet->address = address;

        if (!SDLNet_UDP_Send(socket, -1, packet))
        {
            printf("SDLNet_UDP_Send error: %s\n", SDLNet_GetError());
            return 0;
        }
    }
    return 1;
}

int send_tcp_data(TCPsocket socket, const void *data, int size)
{
    const char *bytes = (const char *)data;
    int sent_total = 0;

    while (sent_total < size)
    {
        int sent = SDLNet_TCP_Send(socket, bytes + sent_total, size - sent_total);
        if (sent <= 0)
        {
            printf("SDLNet_TCP_Send error: %s\n", SDLNet_GetError());
            return 0;
        }
        sent_total += sent;
    }

    return 1;
}

int packet_has_size(UDPpacket *packet, int expectedsize, const char *label)
{
    if (!packet)
    {
        printf("ERROR: packet is NULL\n");
        return 0;
    }

    if (packet->len < expectedsize)
    {
        if (label)
        {
            printf("ERROR: %s packet too small\n", label);
        }
        else
        {
            printf("ERROR: packet too small\n");
        }
        return 0;
    }

    return 1;
}
