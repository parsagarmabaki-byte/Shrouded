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

static TaskMarker markers[] =
{
    { TASK_HOLD,          {320, 140, 24, 24},  208,  304 },
    { TASK_LETTER,        {453, 310, 24, 24},  710, 970 },
    { TASK_CLICK,         {835, 310, 24, 24}, 1950, 940 },
    { TASK_MEMORY,        {675, 140, 24, 24}, 1392,  304 },
    { TASK_ALTERNATE,     {790, 130, 24, 24}, 1770,  304 },
    { TASK_LOGICAL_ORDER, {900, 130, 24, 24}, 2183,  304 },
    { TASK_TIMER,         {825, 570, 24, 24}, 1920, 1795 },
    { TASK_REFLEX,        {415, 505, 24, 24},  570, 1505 }
};


typedef struct GameContext {
    Client *client;
    SDL_Renderer *renderer;
    gameState *state;

    Player *player;
    Task *task;
    GameAssets assets;
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

int runGame(Client *client, waitForPlayers *lobby, gameState *state);

#endif