#ifndef GAME_RENDER_H
#define GAME_RENDER_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "network_data.h"
#include "game_map.h"
#include "game.h"

void render_game_phase(GameContext *ctx);
void render_game_show_role(SDL_Renderer *renderer, SDL_Texture *game_role);
void render_game_info_meeting(GameContext *ctx);
void render_crewmate_win_screen(SDL_Renderer *renderer, GameAssets assets, GameState state);
void render_killer_win(SDL_Renderer *renderer, GameAssets assets, GameState state);
void render_world(GameContext *ctx);
void render_player_overlays(GameContext *ctx);
void render_game_ui(GameContext *ctx);
void render_pause_menu(SDL_Renderer *renderer, GameAssets assets, bool pause_menu_open);
void handle_phase_transition(GameContext *ctx, GamePhase *previous_phase, Uint32 *win_fade_start);
void render_controls_screen(SDL_Renderer *renderer, GameState *state, int local_id, Text text);
void draw_thick_rect(SDL_Renderer *renderer, SDL_Rect rect, int thickness);
void render_info_text(GameState *state, int local_id, Text text);
void render_all_players(GameState *state, Player *player, GameAssets assets, Camera *cam, SDL_Renderer *renderer, int local_id);
void render_task_map(SDL_Renderer *renderer, GameAssets assets, Player *player, GameState *state);
void render_global_progress_bar(SDL_Renderer *renderer, GameState *state);
void run_animations(float *animation_timer, int *current_frame, InputMsg input, float dt);



#endif