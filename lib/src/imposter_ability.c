#define SDL_MAIN_HANDLED
#include "imposter_ability.h"
#include "game_map.h"
#include "network_data.h"
#include "game.h"

void render_imposter_ability(SDL_Renderer *renderer, SDL_Texture *kill_button_img)
{
    SDL_Rect kill_button = {1400, 800, 442, 181};

    bool hovering = is_hovering(renderer, kill_button);
    if (hovering)
    {
        SDL_SetTextureColorMod(kill_button_img, 255, 220, 220); // lite röd ton
    }
    else
    {
        SDL_SetTextureColorMod(kill_button_img, 255, 255, 255);
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

void request_kill(Client *client, gameState *state)
{

}

// bool is_kill_on_cooldown(const gameState *state, int local_id)
// {
//     return false;
// }