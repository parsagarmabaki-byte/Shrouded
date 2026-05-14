#ifndef GAME_RENDER_H
#define GAME_RENDER_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "network_data.h"
#include "game_map.h"
#include "game.h"

void render_game_phase(GameContext *ctx);
static void render_game(GameContext *ctx);
void render_game_show_role(SDL_Renderer *renderer, GameAssets assets, gameState *state, int local_id);
void render_game_info_meeting(GameContext *ctx);
void render_crewmate_win_screen(SDL_Renderer *renderer, GameAssets assets, gameState state);
void render_killer_win(SDL_Renderer *renderer, GameAssets assets, gameState state);
void render_world(GameContext *ctx);
void render_player_overlays(GameContext *ctx);
void render_game_ui(GameContext *ctx);
void render_pause_menu(SDL_Renderer *renderer, GameAssets assets, bool pause_menu_open);
void handle_phase_transition(GameContext *ctx, gamePhase *previous_phase, Uint32 *win_fade_start);
static void render_win_fade(GameContext *ctx, Uint32 win_fade_start, Uint32 win_fade_duration);
void render_controls_screen(SDL_Renderer *renderer, gameState *state, int local_id, Text text);
static void render_task_panel(SDL_Renderer *renderer, gameState *state, int local_id, Task *task, Text panel_text, bool emergency_window_open, bool task_map_open, bool pause_menu_open);
static void render_task_panel(SDL_Renderer *renderer, gameState *state, int local_id, Task *task, Text panel_text, bool emergency_window_open, bool task_map_open, bool pause_menu_open);
void draw_thick_rect(SDL_Renderer *renderer, SDL_Rect rect, int thickness);
void render_info_text(SDL_Renderer *renderer, gameState *state, int local_id, Text text);
void render_all_players(gameState *state, Player *player, GameAssets assets, Camera *cam, SDL_Renderer *renderer, int local_id);
void render_task_map(SDL_Renderer *renderer, Task *task, GameAssets assets, Player *player, gameState *state);
void render_global_progress_bar(SDL_Renderer *renderer, gameState *state);
static const char *task_type_name(TaskType t);
void run_animations(float *animation_timer, int *current_frame, clientInput input, float dt);



#endif