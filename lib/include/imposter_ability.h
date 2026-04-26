#ifndef IMPOSTER_ABILITY
#define IMPOSTER_ABILITY
#include "game_map.h"
#include "game.h"
#include "player_movement.h"
#define COOLDOWN 10000
#define KILL_RADIUS 100

typedef struct
{
    bool active;
    int killer_id;
    int victim_id;
    float x, y;
    int current_frame;
    float animation_timer;
} KillAnimation;


void render_imposter_ability(SDL_Renderer *renderer, gameState state, SDL_Texture *kill_button_active, SDL_Texture *kill_button_deactive,bool kill_cooldown, int killer_id);
bool is_hovering(SDL_Renderer *renderer,SDL_Rect rect);
int handle_kill_request(gameState *state, int killer_id);
void activate_kill_cooldown(gameState *state, int local_id);
bool update_kill_cooldown(gameState state, int local_id);
float find_kill_target(playerState imposter, playerState innocent);
void get_forward_vector(Direction direction, float *fx, float *fy);
void start_kill_animation(KillAnimation *anim, int killer_id, int victim_id, float x, float y);
void update_kill_animation(KillAnimation *anim, float dt);
void render_kill_animation(SDL_Renderer *renderer, KillAnimation *anim, GameAssets assets, Camera *cam);

int target_report_body(KillAnimation bodies[MAX_PLAYERS], Player player);
void render_player_ability(SDL_Renderer *renderer, Player player, GameAssets assets, KillAnimation bodies[MAX_PLAYERS]);


#endif
