#include <stdio.h>
#include <string.h>
#include <SDL2/SDL_net.h>
#include "network.h"
#include "network_data.h"

int init_server(UDPsocket *socket)
{
    if (SDLNet_Init() != 0)
    {
        printf("SDLNet_Init error: %s\n", SDLNet_GetError());
        return 0;
    }

    *socket = SDLNet_UDP_Open(SERVER_PORT); // Server fast port för att klienter ska hitta
    if (!*socket)
    {
        printf("SDLNet_UDP_Open server error: %s\n", SDLNet_GetError());
        return 0;
    }
    return 1;
}

int init_client(UDPsocket *socket, IPaddress *server_addr)
{
    if (SDLNet_Init() != 0)
    {
        printf("SDLNet_Init error: %s\n", SDLNet_GetError());
        return 0;
    }

    *socket = SDLNet_UDP_Open(0); // Hitta slumpmässig ledig
    if (!*socket)
    {
        printf("SDLNet_UDP_Open client error: %s\n", SDLNet_GetError());
        return 0;
    }

    if (SDLNet_ResolveHost(server_addr, "127.0.0.1", SERVER_PORT) != 0)
    {
        printf("SDLNet_ResolveHost error: %s\n", SDLNet_GetError());
        return 0;
    }
    return 1;
}

int send_join(UDPsocket socket, IPaddress server_addr)
{
    UDPpacket *packet = SDLNet_AllocPacket(512);
    if (!packet)
    {
        printf("SDLNet_AllockPacket error: %s\n", SDLNet_GetError());
        return 0;
    }
    clientInput input = {0};
    input.type = MSG_JOIN;
    input.player_id = -1;

    memcpy(packet->data, &input, sizeof(clientInput));
    packet->len = sizeof(clientInput);
    packet->address = server_addr;

    if (!SDLNet_UDP_Send(socket, -1, packet))
    {
        printf("SDLNet_UDP_Send join error: %s\n", SDLNet_GetError());
        SDLNet_FreePacket(packet);
        return 0;
    }

    SDLNet_FreePacket(packet);
    return 1;
}

int send_client_input(UDPsocket socket, IPaddress server_addr, clientInput *input)
{
    UDPpacket *packet = SDLNet_AllocPacket(512);
    if (!packet)
    {
        printf("SDLNet_AllocPacket error: %s\n", SDLNet_GetError());
        return 0;
    }

    memcpy(packet->data, input, sizeof(clientInput));
    packet->len = sizeof(clientInput);
    packet->address = server_addr;

    if (!SDLNet_UDP_Send(socket, -1, packet))
    {
        printf("SDLNet_UDP_Send input error: %s\n", SDLNet_GetError());
        SDLNet_FreePacket(packet);
        return 0;
    }
    SDLNet_FreePacket(packet);
    return 1;
}

int recieve_client_input(UDPsocket socket, UDPpacket *packet, clientInput *input)
{
    if (SDLNet_UDP_Recv(socket, packet))
    {
        memcpy(input, packet->data, sizeof(clientInput));
        return 1;
    }
    return 0;
}

