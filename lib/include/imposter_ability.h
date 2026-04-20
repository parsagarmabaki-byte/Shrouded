#ifndef IMPOSTER_ABILITY
#define IMPOSTER_ABILITY
#include "game_map.h"

void render_imposter_ability(SDL_Renderer *renderer, SDL_Texture *kill_button_img);
bool is_hovering(SDL_Renderer *renderer,SDL_Rect rect);
// bool is_kill_on_cooldown(const gameState *state, int local_id);


#endif
