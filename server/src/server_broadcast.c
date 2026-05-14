#include "server_broadcast.h"
#include "network.h"
#include <stdio.h>

static int send_game_state(UDPsocket socket, UDPpacket *packet, IPaddress addr, gameState *state)
{
    return send_packet_data(socket, packet, addr, state, sizeof(*state));
}

static int send_kill_msg(UDPsocket socket, UDPpacket *packet, IPaddress address, KillEventMsg *msg)
{
    return send_packet_data(socket, packet, address, msg, sizeof(*msg));
}

int init_server_socket(UDPsocket *socket)
{
    return init_network_socket(socket, SERVER_PORT);
}

void cleanupServer(UDPsocket server_socket, UDPpacket *receive_packet, UDPpacket *send_packet)
{
    if (receive_packet)
        SDLNet_FreePacket(receive_packet);
    if (send_packet)
        SDLNet_FreePacket(send_packet);
    if (server_socket)
        SDLNet_UDP_Close(server_socket);
    SDLNet_Quit();
}

void broadcastGameState(UDPsocket socket, UDPpacket *packet, gameState *state, IPaddress *clientAddresses, int *clientUsed)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i])
        {
            state->local_player_id = i;
            if (!send_game_state(socket, packet, clientAddresses[i], state))
            {
                printf("Failed to send game state to player %d\n", i);
            }
        }
    }
}

void broadcast_Kill_msg(UDPsocket socket, UDPpacket *packet, KillEventMsg *msg, IPaddress *clientAddresses, int *clientUsed)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i])
        {
            if (!send_kill_msg(socket, packet, clientAddresses[i], msg))
            {
                printf("Failed to send kill msg to player %d\n", i);
            }
        }
    }
}

void broadcast_emergency_meeting_msg(UDPsocket socket, UDPpacket *packet, KillEventMsg *msg, IPaddress *clientAddresses, int *clientUsed)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i])
        {
            if (!send_kill_msg(socket, packet, clientAddresses[i], msg))
            {
                printf("Failed to send emergency meeting msg to player %d\n", i);
            }
        }
    }
}
