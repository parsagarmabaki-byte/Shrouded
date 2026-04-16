#define SDL_MAIN_HANDLED
#include <SDL2/SDL_net.h>
#include <stdio.h>
#include <string.h>
#include "network.h"
#include "network_data.h"
#include "player_movement.h"
#define PACKET_SIZE 512

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
        SDLNet_FreePacket(receive_packet);
    if (send_packet)
        SDLNet_FreePacket(send_packet);
    if (server_socket)
        SDLNet_UDP_Close(server_socket);
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
    float spawnX[MAX_PLAYERS] = {1290, 1150, 1420, 1000, 1290, 1150};
    float spawnY[MAX_PLAYERS] = {665, 665, 850, 850, 1000, 1000};

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!clientUsed[i])
        {
            clientUsed[i] = 1;
            clientAddresses[i] = addr;

            state->players[i].active = 1;
            state->players[i].player_id = i;
            state->players[i].x = spawnX[i];
            state->players[i].y = spawnY[i];
            state->players[i].current_frame = 2;
            state->players[i].direction = DIR_DOWN;
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
            if (!send_game_state(socket, packet, clientAddresses[i], state))
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

void handleInput(gameState *state, clientInput *input, float dt)
{
    int id = input->player_id;
    if (id < 0 || id >= MAX_PLAYERS)
        return;
    if (!state->players[id].active)
        return;
    if (input->player_id == -1)
        return;

    state->players[id].current_frame = input->current_frame;
    state->players[id].direction = input->direction;

    apply_movement(&state->players[id].x, &state->players[id].y, *input, dt);
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

    // Spara senaste input per spelare
    clientInput lastInput[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        lastInput[i].player_id = -1;
    }

    if (!init_server(&server_socket))
        return 1;

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

    Uint64 lastBroadcast = SDL_GetPerformanceCounter();

    while (1)
    {
        if (SDLNet_UDP_Recv(server_socket, receive_packet))
        {
            MessageType type;
            memcpy(&type, receive_packet->data, sizeof(MessageType));

            if (type == MSG_JOIN)
            {
                int existing = findClientByAddress(clientAddresses, clientUsed, receive_packet->address);
                if (existing < 0)
                {
                    int newPlayer = addToLobby(&state, clientAddresses, clientUsed, receive_packet->address);
                    if (newPlayer >= 0)
                    {
                        printf("Player %d joined\n", newPlayer);
                        broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                    }
                }
            }
            else if (type == MSG_LEAVE)
            {
                int removed = removeFromLobby(&state, clientAddresses, clientUsed, receive_packet->address);
                if (removed >= 0)
                {
                    printf("Player %d left\n", removed);
                    broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                }
            }
            else if (type == MSG_START_GAME)
            {
                if (state.phase == GAME_LOBBY)
                {
                    state.phase = GAME_RUNNING;
                    printf("Game is now GAME_RUNNING\n");
                    broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                }
            }
            else if (type == MSG_CLIENT_INPUT)
            {
                if (state.phase == GAME_RUNNING)
                {
                    clientInput input;
                    memcpy(&input, receive_packet->data, sizeof(clientInput));
                    int id = input.player_id;
                    if (id >= 0 && id < MAX_PLAYERS)
                        lastInput[id] = input;
                }
            }
        }

        // Applicera input och broadcasta på fast 60fps
        Uint64 now = SDL_GetPerformanceCounter();
        float broadcastDt = (float)(now - lastBroadcast) / (float)SDL_GetPerformanceFrequency();

        if (broadcastDt >= SERVER_TICK_INTERVAL)
        {
            if (state.phase == GAME_RUNNING)
            {
                for (int i = 0; i < MAX_PLAYERS; i++)
                {
                    handleInput(&state, &lastInput[i], 0.016f);

                    lastInput[i].player_id = -1;
                    lastInput[i].up = 0;
                    lastInput[i].down = 0;
                    lastInput[i].left = 0;
                    lastInput[i].right = 0;
                    lastInput[i].current_frame = 0;
                    lastInput[i].direction = DIR_DOWN;
                }
                broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
            }
            lastBroadcast = now;
        }
    }

    cleanupServer(server_socket, receive_packet, send_packet);
    return 0;
}