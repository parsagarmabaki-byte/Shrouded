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

void runGame(Client *client, waitForPlayers *lobby, gameState *state);
clientInput read_input(bool tasks_active);
static const char *task_type_name(TaskType t);
static void render_task_panel(SDL_Renderer *renderer, GameAssets assets, gameState *state, int local_id, Task *task, Text panel_text, bool emergency_window_open, bool task_map_open, bool pause_menu_open);
void run_animations(float *animation_timer, int *current_frame, clientInput input, float dt);
void render_all_players(gameState *state, Player *player, GameAssets assets, Camera *cam, SDL_Renderer *renderer, int local_id);
void kill_events(Client *client, SDL_Renderer *renderer, gameState *state, SDL_Event *event, bool kill_cooldown, bool is_local_impostor);
void emergency_meeting_events(Client *client, gameState *state, SDL_Renderer *renderer, SDL_Event *event, Player *player, bool *emergency_window_open, int local_id);
void task_events(SDL_Renderer *renderer, SDL_Event *event, Task *task, Player *player);
void debug_walls(SDL_Renderer *renderer, Camera cam);
void render_task_map(SDL_Renderer *renderer, Task *task, GameAssets assets, Player *player);
void update_player_movement(Player *player, clientInput *user_input, bool task_is_active, bool emergency_window_open, float *accumulator);
void update_player_direction(Player *player, clientInput *user_input);
void render_game_phase(Client *client, SDL_Renderer *renderer, gameState *state, Player *player, Task *task, KillAnimation bodies[MAX_PLAYERS], Camera *cam, GameAssets assets, clientInput user_input, int local_id, bool is_local_impostor, bool task_map_open, bool task_panel_visible, bool *emergency_window_open, float dt, int targeted_banner_id, bool pause_menu_open, Text panel_text);
static void render_game(SDL_Renderer *renderer, gameState *state, Camera *cam, GameAssets assets, clientInput user_input, Player *player, KillAnimation bodies[MAX_PLAYERS], Task *task, int local_id, float dt, bool is_local_impostor, bool emergency_window_open, bool task_map_open, bool pause_menu_open, bool task_panel_visible, Text panel_text);
void process_events(Client *client, SDL_Renderer *renderer, gameState *state, Task *task, SDL_Event *event, Player *player, KillAnimation bodies[MAX_PLAYERS], int local_id, bool *running, bool *emergency_window_open, bool is_local_impostor, bool *task_map_open, bool *task_panel_visible, int *targeted_banner_id, bool *pause_menu_open, GameAssets assets);
void report_body_events(SDL_Renderer *renderer, Client *client, gameState *state, SDL_Event *event, KillAnimation bodies[MAX_PLAYERS], Player *player);
void send_player_input(Client *client, gameState *state, Player *player, bool task_is_active, bool emergency_window_open);
void game_meeting_events(Client *client, SDL_Renderer *renderer, gameState state, SDL_Event *event, int player_alive, int *targeted_banner_id);
void update_task_check_completion(Client *client, Task *task, gameState *state, int local_id, float dt, bool *was_task_active);
void game_running_events(Client *client, SDL_Renderer *renderer, gameState *state, Task *task, SDL_Event *event, Player *player, KillAnimation bodies[MAX_PLAYERS], int local_id, bool *running, bool *emergency_window_open,bool is_local_impostor, bool *task_map_open, bool *task_panel_visible);
void update_game(Client *client, gameState *state, Player *player, Task *task, KillAnimation bodies[MAX_PLAYERS], clientInput *user_input, int local_id, bool ui_open, bool *was_task_active ,float dt, float *accumulator);
float calculate_delta_time(Uint64 *last_tick);
void leave_game_event(Client *client, SDL_Renderer *renderer, SDL_Event *event, bool *running, bool *emergency_window_open, bool *pause_menu_open, GameAssets assets);



#endif
