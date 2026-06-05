#ifndef IMPOSTER_ABILITY
#define IMPOSTER_ABILITY
#include "game_map.h"
#include "game.h"
#include "player_movement.h"
#include "kill_animation.h"
#define COOLDOWN 30000
#define KILL_RADIUS 100


void render_imposter_ability(SDL_Renderer *renderer, gameState state, SDL_Texture *kill_button_active, SDL_Texture *kill_button_deactive,bool kill_cooldown, int killer_id);
bool is_hovering(SDL_Renderer *renderer,SDL_Rect rect);
int handle_kill_request(gameState *state, int killer_id);
void activate_kill_cooldown(Uint64 *kill_cooldown_start, bool *kill_cooldown_active, int killer_id);
void update_kill_cooldown(UDPsocket socket, UDPpacket *packet, IPaddress address, Uint64 *kill_cooldown_start, bool *kill_cooldown);
float find_kill_target(playerState imposter, playerState innocent);
void get_forward_vector(Direction direction, float *fx, float *fy);
void start_kill_animation(KillAnimation *anim, int killer_id, int victim_id, float x, float y);
void update_kill_animation(KillAnimation bodies[MAX_PLAYERS], float dt);
void render_kill_animation(SDL_Renderer *renderer, KillAnimation bodies[MAX_PLAYERS], GameAssets assets, Camera *cam);
int target_report_body(KillAnimation bodies[MAX_PLAYERS], Player player);
void render_player_ability(SDL_Renderer *renderer, Player player, GameAssets assets, KillAnimation bodies[MAX_PLAYERS]);
bool find_target_report_body(Position bodies, int player_x, int player_y);


#endif
