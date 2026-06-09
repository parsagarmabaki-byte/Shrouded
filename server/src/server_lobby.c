#include "server_lobby.h"
#include <stdbool.h>

int get_player_id_from_sender(IPaddress *clientAddresses, int *clientUsed, IPaddress sender)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i] &&
            clientAddresses[i].host == sender.host &&
            clientAddresses[i].port == sender.port)
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
            state->players[i].isKiller = 0;
            state->kill_cooldown_active = false;
            state->players[i].emergency_meeting = 1;
            state->emergency_meeting_reported_id = -1;
            return i;
        }
    }
    return -1;
}

int removeFromLobby(gameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr)
{
    int player = get_player_id_from_sender(clientAddresses, clientUsed, addr);
    if (player >= 0)
    {
        clientUsed[player] = 0;
        state->players[player].active = 0;
        state->players[player].player_id = -1;
        return player;
    }
    return -1;
}

void spawn_players(gameState *state)
{
    float spawnX[MAX_PLAYERS] = {1290, 1150, 1420, 1000, 1290, 1150};
    float spawnY[MAX_PLAYERS] = {665, 665, 850, 850, 1000, 1000};
    state->kill_cooldown_active = false;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        state->players[i].x = spawnX[i];
        state->players[i].y = spawnY[i];
        state->players[i].current_frame = 2;
        state->players[i].direction = DIR_DOWN;
    }
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
