#include <SDL2/SDL_net.h>
#include <stdio.h>
#include <string.h>
#include "network.h"
#include "network_data.h"

#define PACKET_SIZE 512

int initServerNetwork(UDPsocket *server_socket)
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
UDPpacket *createPacket(int size)
{
    UDPpacket *packet = SDLNet_AllocPacket(size);
    if (!packet)
    {
        printf("Failed to allocate packet: %s\n", SDLNet_GetError());
    }
    return packet;
}

int receiveJoinMessage(UDPsocket server_socket, UDPpacket *receive_packet, joinMessage *join)
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

void cleanupServer(UDPsocket server_socket, UDPpacket *receive_packet, UDPpacket *send_packet)
{
    if (receive_packet)
    {
        SDLNet_FreePacket(receive_packet);
    }

    if (send_packet)
    {
        SDLNet_FreePacket(send_packet);
    }
    if (server_socket)
    {
        SDLNet_UDP_Close(server_socket);
    }


    SDLNet_Quit();
}

int countActivePlayers(gameState *state)
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
int findClientByAddress(IPaddress *clientAddresses, int *clientUsed, IPaddress addr)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i] && clientAddresses[i].host == addr.host && clientAddresses[i].port == addr.port)
        {
            return i;
        }
    }
    return -1;
    
}
int addToLobby(gameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!clientUsed[i])
        {
            clientUsed[i] = 1;
            clientAddresses[i] = addr;

            state->players[i].active = 1;
            state->players[i].player_id = i;
            return i;
        }
    }
    return -1;
}
void broadcastGameState(UDPsocket socket, UDPpacket *packet, gameState *state, IPaddress *clientAddresses, int *clientUsed)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i])
        {
            state->local_player_id = i;

            if (!send_game_state(socket,packet, clientAddresses[i], state))
            {
                printf("Failed to send game state to player %d\n", i);
            }
        }
    }
    
}
int main(void)
{
    UDPsocket server_socket = NULL;
    UDPpacket *receive_packet = NULL;
    UDPpacket *send_packet = NULL;

    gameState state = {0};
    state.type = MSG_GAME_STATE;
    state.phase = GAME_LOBBY;

    IPaddress clientAddresses[MAX_PLAYERS];
    int clientUsed[MAX_PLAYERS] = {0};

    if (!initServerNetwork(&server_socket))
    {
        return 1;
    }

    receive_packet = createPacket(PACKET_SIZE);
    if (!receive_packet)
    {
        cleanupServer(server_socket, NULL, NULL);
        return 1;
    }
    send_packet = createPacket(PACKET_SIZE);
    if (!send_packet)
    {
        cleanupServer(server_socket, receive_packet, NULL);
        return 1;
    }

    printf("Server listening on port %d...\n", SERVER_PORT);
    printf("Waiting for clients... %d/%d\n", countActivePlayers(&state), MAX_PLAYERS);

    while (1)
    {
        joinMessage join;

        if (receiveJoinMessage(server_socket, receive_packet, &join))
        {
            if (join.type == MSG_JOIN)
            {
                int existingPlayer = findClientByAddress(clientAddresses,clientUsed, receive_packet->address);

                if (existingPlayer >= 0)
                {
                    printf("Client already connected as player %d\n", existingPlayer);
                }
                else 
                {
                    int newPlayer = addToLobby(&state, clientAddresses, clientUsed, receive_packet->address);

                    if (newPlayer >= 0)
                    {
                        printf("Client joined as player %d\n", newPlayer);
                        printf("Players connected: %d/%d\n", countActivePlayers(&state), MAX_PLAYERS);

                        broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                    }
                    else
                    {
                        printf("Lobby full.\n");
                    }
                }
            }
         }
    }
    cleanupServer(server_socket, receive_packet, send_packet);
    return 0;
}