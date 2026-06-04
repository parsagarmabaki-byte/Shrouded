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
#include <time.h>
#include "task.h"
#include "kill_animation.h"
#include "imposter_ability.h"
#include "text.h"
#include "task_render.h"
#include "task_init.h"
#include "game_input.h"
#include "SFX.h"

typedef struct Client
{
    UDPsocket socket;
    IPaddress serverAddr;
    UDPpacket *recievepacket;
} Client;

typedef struct
{
    TaskType type;
    SDL_Rect rect;
    int world_x;
    int world_y;
} TaskMarker;

typedef struct GameContext {
    Client *client;
    SDL_Renderer *renderer;
    gameState *state;
    AudioAssets *audio;

    Player *player;
    Task *task;
    GameAssets assets;
    // Game_Show_Role_asset show_role_asset;
    SDL_Texture *player_role;
    KillAnimation bodies[MAX_PLAYERS];
    Camera cam;

    Text small_text;
    Text generic_text;
    Text timer_meeting_text;

    SDL_Event event;
    clientInput user_input;

    int local_id;
    int targeted_banner_id;
    int player_voted;

    bool running;
    bool return_to_menu;
    bool emergency_window_open;
    bool is_local_impostor;
    bool task_map_open;
    bool pause_menu_open;
    bool task_panel_visible;
    bool controls_visible;
    bool was_task_active;
    bool ui_open;

    float accumulator;
    float dt;
    Uint64 last_tick;
} GameContext;

int runGame(Client *client, waitForPlayers *lobby, gameState *state, AudioAssets *audio);

#endif
