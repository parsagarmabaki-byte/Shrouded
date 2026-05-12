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

typedef struct
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



int runGame(Client *client, waitForPlayers *lobby, gameState *state);
clientInput read_input(bool tasks_active);
void render_controls_screen(SDL_Renderer *renderer, gameState *state, int local_id, Text text);
static const char *task_type_name(TaskType t);
static void render_task_panel(SDL_Renderer *renderer, gameState *state, int local_id, Task *task, Text panel_text, bool emergency_window_open, bool task_map_open, bool pause_menu_open);
void draw_thick_rect(SDL_Renderer *renderer, SDL_Rect rect, int thickness);
void render_info_text(SDL_Renderer *renderer, gameState *state, int local_id, Text text);
void run_animations(float *animation_timer, int *current_frame, clientInput input, float dt);
void render_all_players(gameState *state, Player *player, GameAssets assets, Camera *cam, SDL_Renderer *renderer, int local_id);
void kill_events(Client *client, SDL_Renderer *renderer, gameState *state, SDL_Event *event, bool kill_cooldown, bool is_local_impostor);
void emergency_meeting_events(Client *client, gameState *state, SDL_Renderer *renderer, SDL_Event *event, Player *player, bool *emergency_window_open, int local_id);
void task_events(SDL_Renderer *renderer, SDL_Event *event, Task *task, Player *player, bool is_local_impostor, gameState *state, int local_id);
void debug_walls(SDL_Renderer *renderer, Camera cam);
void render_task_map(SDL_Renderer *renderer, Task *task, GameAssets assets, Player *player, gameState *state);
void update_player_movement(Player *player, clientInput *user_input, bool task_is_active, bool emergency_window_open, float *accumulator);
void update_player_direction(Player *player, clientInput *user_input);
void render_game_phase(Client *client, SDL_Renderer *renderer, gameState *state, Player *player, Task *task, KillAnimation bodies[MAX_PLAYERS], Camera *cam, GameAssets assets, clientInput user_input, int local_id, bool is_local_impostor, bool task_map_open, bool task_panel_visible, bool *emergency_window_open, float dt, int targeted_banner_id, bool pause_menu_open, Text panel_text, bool controls_visible, Text generic_text,Text timer_meeting_text);
static void render_game(SDL_Renderer *renderer, gameState *state, Camera *cam, GameAssets assets, clientInput user_input, Player *player, KillAnimation bodies[MAX_PLAYERS], Task *task, int local_id, float dt, bool is_local_impostor, bool emergency_window_open, bool task_map_open, bool pause_menu_open, bool task_panel_visible, Text panel_text, bool controls_visible, Text generic_text);
void process_events(Client *client, SDL_Renderer *renderer, gameState *state, Task *task, SDL_Event *event, Player *player, KillAnimation bodies[MAX_PLAYERS], int local_id, bool *running, bool *return_to_menu, bool *emergency_window_open, bool is_local_impostor, bool *task_map_open, bool *task_panel_visible, int *targeted_banner_id, bool *pause_menu_open, bool *controls_visible, GameAssets assets);
void report_body_events(SDL_Renderer *renderer, Client *client, gameState *state, SDL_Event *event, KillAnimation bodies[MAX_PLAYERS], Player *player, Task *task);
void send_player_input(Client *client, gameState *state, Player *player, bool task_is_active, bool emergency_window_open);
void game_meeting_events(Client *client, SDL_Renderer *renderer, gameState state, SDL_Event *event, int player_alive, int *targeted_banner_id);
void update_task_check_completion(Client *client, Task *task, gameState *state, int local_id, float dt, bool *was_task_active);
void game_running_events(Client *client, SDL_Renderer *renderer, gameState *state, Task *task, SDL_Event *event, Player *player, KillAnimation bodies[MAX_PLAYERS], int local_id, bool *running, bool *emergency_window_open,bool is_local_impostor, bool *task_map_open, bool *task_panel_visible, bool *controls_visible);
void update_game(Client *client, gameState *state, Player *player, Task *task, KillAnimation bodies[MAX_PLAYERS], clientInput *user_input, int local_id, bool ui_open, bool *was_task_active ,float dt, float *accumulator);
float calculate_delta_time(Uint64 *last_tick);
void leave_game_event(Client *client, SDL_Renderer *renderer, SDL_Event *event, bool *running, bool *emergency_window_open, bool *pause_menu_open, GameAssets assets);



#endif