#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include <SDL2/SDL_net.h>
#include "network_data.h"
#include "lobby.h"
#include "network.h"
#include "game_map.h"
#include "player_movement.h"
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

typedef struct player{
    SDL_FRect Hitbox;
    Direction direction;
    int current_frame;
    float animation_timer;
}Player;

// Forward declare Player to break circular dependency
typedef struct player Player;

typedef struct
{
    UDPsocket socket;
    IPaddress serverAddr;
    UDPpacket *recievepacket;
} Client;

void sendInput(Client *client, gameState *state, Player *player);
void runGame(Client *client, waitForPlayers *lobby, gameState *state);
void collect_client_data(Client *client ,gameState *state,Player *player, int local_id);
clientInput read_input(void);
void run_animations(float *animation_timer, int *current_frame, clientInput input, float dt);
void render_all_players(gameState *state,Player player,GameAssets assets, Camera *cam, SDL_Renderer *renderer, int local_id);




#endif
