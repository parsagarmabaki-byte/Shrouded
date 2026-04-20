#define SDL_MAIN_HANDLED
#include "imposter_ability.h"
#include "game_map.h"
#include "network_data.h"
#include "game.h"

void render_imposter_ability(SDL_Renderer *renderer, SDL_Texture *kill_button_img, bool kill_cooldown)
{
    SDL_Rect kill_button = {1400, 800, 442, 181};

    SDL_SetTextureColorMod(kill_button_img, 255, 255, 255);
    SDL_SetTextureAlphaMod(kill_button_img, 255);

    if (!kill_cooldown)
    {
        bool hovering = is_hovering(renderer, kill_button);

        if (hovering)
        {
            SDL_SetTextureColorMod(kill_button_img, 255, 220, 220);
        }
    }
    else
    {
        SDL_SetTextureAlphaMod(kill_button_img, 150);
    }

    SDL_RenderCopy(renderer, kill_button_img, NULL, &kill_button);
}
bool is_hovering(SDL_Renderer *renderer, SDL_Rect rect)
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    float lx, ly;
    SDL_RenderWindowToLogical(renderer, mx, my, &lx, &ly);

    return (lx >= rect.x &&
            lx <= rect.x + rect.w &&
            ly >= rect.y &&
            ly <= rect.y + rect.h);
}

bool handle_kill_request(gameState *state, int killer_id)
{
    playerState imposter = state->players[killer_id];
    if (imposter.kill_cooldown_active)
    {
        return false;
    }
    activate_kill_cooldown(state, killer_id);
    for (int i=0; i<MAX_PLAYERS; i++)
    {
        if (killer_id == i)
        {
            continue;
        }
        if (state->players[i].isAlive && state->players[i].active)
        {
            float fy,fx = 0;
            get_forward_vector(imposter.direction,&fx,&fy);
            

        }
    }
    
}

void get_forward_vector(Direction direction, float *fx, float *fy)
{
    *fx = 0.0f;
    *fy = 0.0f;

    if (direction == DIR_UP)    *fy = -1.0f;
    if (direction == DIR_DOWN)  *fy =  1.0f;
    if (direction == DIR_LEFT)  *fx = -1.0f;
    if (direction == DIR_RIGHT) *fx =  1.0f;
}

void activate_kill_cooldown(gameState *state, int local_id)
{
    state->players[local_id].kill_cooldown_start = SDL_GetTicks64();
    state->players[local_id].kill_cooldown_active = true;
}

bool update_kill_cooldown(gameState state, int local_id)
{
    Uint64 now = SDL_GetTicks64();
    if (now - state.players[local_id].kill_cooldown_start >= COOLDOWN)
        return false;
    return true;
}