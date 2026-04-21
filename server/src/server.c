#define SDL_MAIN_HANDLED
#include <SDL2/SDL_net.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "network.h"
#include "network_data.h"
#include "player_movement.h"
#include "imposter_ability.h"
#define PACKET_SIZE 512

static int init_server(UDPsocket *socket)
{
    return init_network_socket(socket, SERVER_PORT);
}

static int send_game_state(UDPsocket socket, UDPpacket *packet, IPaddress addr, gameState *state)
{
    return send_packet_data(socket, packet, addr, state, sizeof(*state));
}

static int send_kill_msg(UDPsocket socket, UDPpacket *packet, IPaddress address, KillEventMsg *msg)
{
    return send_packet_data(socket, packet, address, msg, sizeof(*msg));
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
    float spawnX[MAX_PLAYERS] = {1290, 1150, 1420, 1000, 1290, 1150};
    float spawnY[MAX_PLAYERS] = {665, 665, 850, 850, 1000, 1000};

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!clientUsed[i])
        {
            clientUsed[i] = 1;
            clientAddresses[i] = addr;

            state->players[i].active = 1;
            state->players[i].isAlive = 1;
            state->players[i].player_id = i;
            state->players[i].x = spawnX[i];
            state->players[i].y = spawnY[i];
            state->players[i].current_frame = 2;
            state->players[i].direction = DIR_DOWN;
            state->players[i].isImpostor = 0;
            state->players[i].kill_cooldown_start = 0;
            state->players[i].kill_cooldown_active = false;
            return i;
        }
    }
    return -1;
}

int countActivePlayers(gameState *state)
{
    int active_players = 0;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (state->players[i].active)
            active_players++;
    }

    return active_players;
}

int designateImpostor(gameState *state)
{
    int active_player_count = countActivePlayers(state);
    int chosen_active_player = 0;
    int active_player_index = 0;

    chosen_active_player = rand() % active_player_count;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!state->players[i].active)
            continue;

        if (active_player_index == chosen_active_player)
        {
            state->players[i].isImpostor = 1;
            return chosen_active_player;
        }

        active_player_index++;
    }
    return -1; // alla aktiva spelare gicks igenom utan match (borde inte hända)
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
            if (!send_kill_msg(socket, packet ,clientAddresses[i], msg))
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
    srand(time(NULL));

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

    receive_packet = create_packet(PACKET_SIZE);
    if (!receive_packet)
    {
        cleanupServer(server_socket, NULL, NULL);
        return 1;
    }

    send_packet = create_packet(PACKET_SIZE);
    if (!send_packet)
    {
        cleanupServer(server_socket, receive_packet, NULL);
        return 1;
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    Uint64 lastBroadcast = SDL_GetPerformanceCounter();
    Uint64 state_start_time = 0;

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
                    int active_chosen_player = designateImpostor(&state);
                    printf("Player %d is impostor\n", active_chosen_player);
                    state.phase = GAME_SHOW_ROLE;
                    printf("Game is now GAME_SHOW_ROLE\n");

                    state_start_time = SDL_GetTicks64(); // TIDSSTÄMPEL

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
            else if (type == MSG_KILL_REQUEST)
            {
                clientInput input;
                memcpy(&input, receive_packet->data, sizeof(clientInput));
                int killer_id = input.player_id;
                if (killer_id >= 0 && killer_id < MAX_PLAYERS)
                {
                    int target_id = handle_kill_request(&state, killer_id);
                    if (target_id != -1)
                    {
                        state.players[target_id].isAlive = 0;
                        KillEventMsg msg = {0};
                        msg.type = MSG_KILL_EVENT;
                        msg.killer_id = killer_id;
                        msg.victim_id = target_id;
                        msg.x = state.players[killer_id].x;
                        msg.y = state.players[killer_id].y;
                        broadcast_Kill_msg(server_socket, send_packet, &msg, clientAddresses, clientUsed);
                    }
                }
            }
        }

        // Applicera input och broadcasta på fast 60fps
        Uint64 now = SDL_GetPerformanceCounter();
        float broadcastDt = (float)(now - lastBroadcast) / (float)SDL_GetPerformanceFrequency();

        if (broadcastDt >= SERVER_TICK_INTERVAL)
        {
            if (state.phase == GAME_SHOW_ROLE)
            {
                if (SDL_GetTicks64() - state_start_time >= 3000) // NÄR 3 SEKUNDER GÅTT
                {
                    state.phase = GAME_RUNNING;
                    printf("Game is now GAME_RUNNING\n");
                }
                broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
            }
            else if (state.phase == GAME_RUNNING)
            {
                for (int i = 0; i < MAX_PLAYERS; i++)
                {
                    handleInput(&state, &lastInput[i], 0.016f);
                    if (state.players[i].kill_cooldown_active)
                    {
                        state.players[i].kill_cooldown_active = update_kill_cooldown(state, i);
                    }
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
