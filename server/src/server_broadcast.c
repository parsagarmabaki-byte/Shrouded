#include "server_broadcast.h"
#include "network.h"
#include <stdio.h>

static int send_packet(UDPsocket socket, UDPpacket *packet, IPaddress addr, const void *data, size_t size)
{
    return send_packet_data(socket, packet, addr, data, size);
}

int init_server_socket(UDPsocket *socket, TCPsocket *tcp_socket)
{
    if (!init_network_socket(socket, SERVER_PORT))
        return 0;

    if (!init_tcp_server_socket(tcp_socket, SERVER_TCP_PORT))
    {
        SDLNet_UDP_Close(*socket);
        *socket = NULL;
        return 0;
    }

    return 1;
}

void cleanupServer(UDPsocket socket, TCPsocket tcp_socket, SDLNet_SocketSet socket_set, TCPsocket *tcp_sockets, UDPpacket *recv, UDPpacket *send)
{
    if (tcp_sockets)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (tcp_sockets[i])
                SDLNet_TCP_Close(tcp_sockets[i]);
        }
    }
    if (socket_set)
        SDLNet_FreeSocketSet(socket_set);
    if (recv)
        SDLNet_FreePacket(recv);
    if (send)
        SDLNet_FreePacket(send);
    if (tcp_socket)
        SDLNet_TCP_Close(tcp_socket);
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

void broadcast_tcp_msg(TCPsocket *sockets, const void *data, size_t size)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (sockets[i] && !send_tcp_data(sockets[i], data, (int)size))
            printf("Failed to send TCP packet to player %d\n", i);
    }
}
