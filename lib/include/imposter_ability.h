#ifndef IMPOSTER_ABILITY
#define IMPOSTER_ABILITY
#include "game_map.h"
#include "game.h"
#define COOLDOWN 10000

void render_imposter_ability(SDL_Renderer *renderer, SDL_Texture *kill_button_img, bool kill_cooldown);
bool is_hovering(SDL_Renderer *renderer,SDL_Rect rect);
// bool is_kill_on_cooldown(const gameState *state, int local_id);
bool handle_kill_request(gameState *state, int killer_id);
void activate_kill_cooldown(gameState *state, int local_id);
bool update_kill_cooldown(gameState state, int local_id);


#endif
