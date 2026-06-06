#include "server_broadcast.h"
#include "network.h"
#include <stdio.h>

static int send_packet(UDPsocket socket, UDPpacket *packet, IPaddress addr, const void *data, size_t size)
{
    return send_packet_data(socket, packet, addr, data, size);
}

int init_server_socket(UDPsocket *socket)
{
    return init_network_socket(socket, SERVER_PORT);
}

void cleanupServer(UDPsocket socket, UDPpacket *recv, UDPpacket *send)
{
    if (recv)
        SDLNet_FreePacket(recv);
    if (send)
        SDLNet_FreePacket(send);
    if (socket)
        SDLNet_UDP_Close(socket);
    SDLNet_Quit();
}

void broadcast_msg(UDPsocket socket, UDPpacket *packet, IPaddress *clients, int *used, const void *data, size_t size)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
        if (used[i] &&
            !send_packet(socket, packet, clients[i], data, size))
            printf("Failed to send packet to player %d\n", i);
}

void broadcast_game_state(UDPsocket socket, UDPpacket *packet, gameState *state, IPaddress *clients, int *used)
{
    state->type = MSG_GAME_STATE;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!used[i])
            continue;

        state->local_player_id = i;

        if (!send_packet(socket, packet, clients[i], state, sizeof(*state)))
            printf("Failed to send state to player %d\n", i);
    }
}