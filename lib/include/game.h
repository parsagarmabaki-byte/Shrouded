#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include <SDL2/SDL_net.h>
#include "network_data.h"
#include "lobby.h"

// Forward declare Player to break circular dependency
typedef struct player Player;

typedef struct
{
    UDPsocket socket;
    IPaddress serverAddr;
    UDPpacket *recievepacket;
} Client;

#include "player_movement.h"

void sendInput(Client *client, gameState *state);
void runGame(Client *client, waitForPlayers *lobby, gameState *state);
void run_animation(Player *player, float dt);

#endif
