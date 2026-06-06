#include <stdio.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include "game_map.h"
#include "network_data.h"
#include "imposter_ability.h"

#define col_dx 430
#define row_dy 124
#define banner_x 250
#define banner_y 170

#define VOTE_SUBMIT_X 260
#define VOTE_SUBMIT_Y 555
#define VOTE_SUBMIT_W 265
#define VOTE_SUBMIT_H 75

#define VOTE_SKIP_X    760
#define VOTE_SKIP_Y    555
#define VOTE_SKIP_W    265
#define VOTE_SKIP_H    75

void emergency_meeting_view(SDL_Renderer *renderer, SDL_Texture *emergency_button_view, SDL_Texture *emergency_button_hover);
void render_emergency_meeting(SDL_Renderer *renderer, GameAssets assets, gameState *state, int id_reported, int targeted_banner_id, Text timer_meeting_text, int local_id, int *player_voted);
void render_emergency_icon(SDL_Renderer *renderer, SDL_Texture *icon, int id_reported);
void render_banners(SDL_Renderer *renderer, GameAssets assets, gameState *state, int targeted_banner_id, int player_voted);
void render_emergency_map(SDL_Renderer *renderer, GameAssets assets, int player_alive, int player_voted);
int target_player_banner(SDL_Renderer *renderer, gameState state, SDL_Event *event, int player_alive, int target_banner_id);
int handle_send_vote_button(Client *client, SDL_Renderer *renderer, SDL_Event *event, int player_alive, int targeted_banner, int *player_voted);
void render_voting_result_layer(SDL_Renderer *renderer, GameAssets assets, int target_id);
void render_voting_banners(SDL_Renderer *renderer, gameState *state, GameAssets assets);
void render_voting_screen(SDL_Renderer *renderer, gameState *state, GameAssets assets, int voting_result);
void render_voting_results(SDL_Renderer *renderer, gameState *state, int voting_results[MAX_PLAYERS]);


SDL_Rect get_banner_rect(int i);