#define SDL_MAIN_HANDLED
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

int findClientByAddress(IPaddress *clientAddresses, int *clientUsed, IPaddress addr)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i] && 
            clientAddresses[i].host == addr.host && 
            clientAddresses[i].port == addr.port)
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
int removeFromLobby(gameState *state, IPaddress *clientAddress, int *clientUsed, IPaddress addr)
{
    int player = findClientByAddress(clientAddress, clientUsed, addr);

    if (player >= 0)
    {
        clientUsed[player] = 0;
        state->players[player].active = 0;
        state->players[player].player_id = -1;
        return player;
    }
    
    return -1;
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

    while (1)
    {
        if (SDLNet_UDP_Recv(server_socket, receive_packet))
        {
            MessageType type;
            memcpy(&type, receive_packet->data, sizeof(MessageType));
            
            if (type == MSG_JOIN)
            {
                int existingPlayer = findClientByAddress(clientAddresses,clientUsed, receive_packet->address);

                if (existingPlayer < 0)
                {
                    int newPlayer = addToLobby(&state, clientAddresses, clientUsed, receive_packet->address);
                    if (newPlayer >= 0)
                    {
                        broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                    }
                }
            }
            else if (type == MSG_LEAVE)
            {
                int removedPlayer = removeFromLobby(&state, clientAddresses, clientUsed, receive_packet->address);

                if (removedPlayer >= 0)
                {
                    broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                }
            }
            else if (type == MSG_START_GAME)
            {
                if (state.phase == GAME_LOBBY)
                {
                    state.phase = GAME_RUNNING;
                    printf("Game state is now GAME_RUNNING\n");
                    broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                }
            }
        }
    }
    cleanupServer(server_socket, receive_packet, send_packet);
    return 0;
}