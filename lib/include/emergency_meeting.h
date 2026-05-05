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

void emergency_meeting_view(SDL_Renderer *renderer, SDL_Texture *emergency_button_view);
void render_emergency_meeting(SDL_Renderer *renderer, GameAssets assets, gameState *state, int id_reported, int targeted_banner_id);
void render_emergency_icon(SDL_Renderer *renderer, SDL_Texture *icon, int id_reported);
void render_banners(SDL_Renderer *renderer, GameAssets assets, gameState *state, int targeted_banner_id);
void render_emergency_map(SDL_Renderer *renderer, GameAssets assets, int player_alive);
int target_player_banner(SDL_Renderer *renderer, gameState state, SDL_Event *event, int player_alive, int target_banner_id);
int handle_send_vote_button(Client *client, SDL_Renderer *renderer, SDL_Event *event, int player_alive, int targeted_banner);
SDL_Rect get_banner_rect(int i);
