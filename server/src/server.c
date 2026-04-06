#define SDL_MAIN_HANDLED
#include <SDL2/SDL_net.h>
#include <stdio.h>
#include <string.h>
#include "network.h"
#include "network_data.h"

#define PACKET_SIZE 512

int init_server_network(UDPsocket *server_socket)
{
    if (SDLNet_Init() < 0)
    {
        printf("SDLNet_Init failed: %s\n", SDLNet_GetError());
        return 0;
    }

    *server_socket = SDLNet_UDP_Open(SERVER_PORT);
    if (!*server_socket)
    {
        printf("Failed to open server socket: %s\n", SDLNet_GetError());
        SDLNet_Quit();
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

int receive_join_message(UDPsocket server_socket, UDPpacket *receive_packet, joinMessage *join)
{
    if (!SDLNet_UDP_Recv(server_socket, receive_packet))
    {
        return 0;
    }

    if (receive_packet->len < (int)sizeof(joinMessage))
    {
        printf("Received packet too small for joinMessage\n");
        return 0;
    }

    memcpy(join, receive_packet->data, sizeof(joinMessage));
    return 1;
}

void cleanup_server(UDPsocket server_socket, UDPpacket *receive_packet)
{
    if (receive_packet)
    {
        SDLNet_FreePacket(receive_packet);
    }

    if (server_socket)
    {
        SDLNet_UDP_Close(server_socket);
    }

    SDLNet_Quit();
}

int count_active_players(gameState *state)
{
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (state->players[i].active)
        {
            count++;
        }
    }
    return count;
}
int add_to_lobby(gameState *state)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!state->players[i].active)
        {
            state->players[i].active = 1;
            state->players[i].player_id = i;
            return i;
        }
    }
    return -1;
}
int main(void)
{
    UDPsocket server_socket = NULL;
    UDPpacket *receive_packet = NULL;

    gameState state = {0};
    state.type = MSG_GAME_STATE;
    state.phase = GAME_LOBBY;

    if (!init_server_network(&server_socket))
    {
        return 1;
    }

    receive_packet = create_packet(PACKET_SIZE);
    if (!receive_packet)
    {
        cleanup_server(server_socket, NULL);
        return 1;
    }

    printf("Server listening on port %d...\n", SERVER_PORT);
    printf("Waiting for clients... %d/%d\n", count_active_players(&state), MAX_PLAYERS);

    while (1)
    {
        joinMessage join;

        if (receive_join_message(server_socket, receive_packet, &join))
        {
            if (join.type == MSG_JOIN)
            {
                int player_id = add_to_lobby(&state);
                if (player_id >= 0)
                {
                    printf("Client joined as player %d\n", player_id);
                    printf("Waiting for clients... %d/%d\n", count_active_players(&state), MAX_PLAYERS);
                }
                else
                {
                    printf("Lobby full.\n");
                }
            }
        }
    }
    cleanup_server(server_socket, receive_packet);
    return 0;
}