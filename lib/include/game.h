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
#include "task.h"
#include "imposter_ability.h"

typedef struct
{
    UDPsocket socket;
    IPaddress serverAddr;
    UDPpacket *recievepacket;
} Client;
typedef struct
{
    bool active;
    int killer_id;
    int victim_id;
    float x, y;
    int current_frame;
    float animation_timer;
} KillAnimation;

void sendInput(Client *client, gameState *state, Player *player);
void runGame(Client *client, waitForPlayers *lobby, gameState *state);
void collect_packets(Client *client, gameState *state, KillAnimation *bodies);
clientInput read_input(bool tasks_active);
void run_animations(float *animation_timer, int *current_frame, clientInput input, float dt);
void render_all_players(gameState *state, Player player, GameAssets assets, Camera *cam, SDL_Renderer *renderer, int local_id);
void request_kill(Client *client, gameState *state);
void start_kill_animation(KillAnimation *anim, int killer_id, int victim_id, float x, float y);
void update_kill_animation(KillAnimation *anim, float dt);
void render_kill_animation(SDL_Renderer *renderer, KillAnimation *anim, GameAssets assets, Camera *cam);

#endif
