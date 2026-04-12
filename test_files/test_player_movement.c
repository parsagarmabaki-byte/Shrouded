#define SDL_MAIN_HANDLED
#include "player_movement.h"
#include "game_map.h"

int main(void)
{
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    window = SDL_CreateWindow("Test window", 0, 0, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    renderer = SDL_CreateRenderer(window, -1, 0);

    GameAssets game_assets = load_assets(renderer);

    if (!game_assets.skins[0]) {
    printf("Failed to load texture: %s\n", SDL_GetError());
    }

    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);
    Player player = init_player(window_width, window_height);

    movement(window, renderer, &player, window_width, window_height, game_assets.skins[2],game_assets.map_texture);

    // SDL_Quit();
}