#define SDL_MAIN_HANDLED
#include "player_movement.h"
#include "game_map.h"

// int main(void)
// {
//     SDL_SetMainReady();

//     if (SDL_Init(SDL_INIT_VIDEO) != 0)
//     {
//         printf("SDL_Init failed: %s\n", SDL_GetError());
//         return 1;
//     }

//     if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
//     {
//         printf("IMG_Init failed: %s\n", IMG_GetError());
//         SDL_Quit();
//         return 1;
//     }

//     SDL_Window *window = SDL_CreateWindow(
//         "Test window",
//         SDL_WINDOWPOS_CENTERED,
//         SDL_WINDOWPOS_CENTERED,
//         1280,
//         720,
//         SDL_WINDOW_FULLSCREEN_DESKTOP
//     );

//     if (!window)
//     {
//         printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
//         IMG_Quit();
//         SDL_Quit();
//         return 1;
//     }

//     SDL_Renderer *renderer = SDL_CreateRenderer(
//         window,
//         -1,
//         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
//     );

//     if (!renderer)
//     {
//         printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
//         SDL_DestroyWindow(window);
//         IMG_Quit();
//         SDL_Quit();
//         return 1;
//     }

//     int window_width, window_height;
//     SDL_GetWindowSize(window, &window_width, &window_height);
//     printf("Wh: %d, WW %d",window_height, window_width);

//     Player player = init_player(window_width, window_height);

//     SDL_Texture *backgroundTexture = loading_img(renderer, "assets/images/Game_map.png");
//     if (!backgroundTexture)
//     {
//         SDL_DestroyRenderer(renderer);
//         SDL_DestroyWindow(window);
//         IMG_Quit();
//         SDL_Quit();
//         return 1;
//     }

//     movement(window, renderer, backgroundTexture, &player, window_width, window_height);

//     SDL_DestroyTexture(backgroundTexture);
//     SDL_DestroyRenderer(renderer);
//     SDL_DestroyWindow(window);
//     IMG_Quit();
//     SDL_Quit();

//     return 0;
// }

int main(void)
{
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    window = SDL_CreateWindow("Test window", 0, 0, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Texture* playerTexture = IMG_LoadTexture(renderer, "assets/sprites/charspritesv2.png");
    SDL_Texture *background_texture = loading_img(renderer, "assets/images/Game_map.png");

    if (!playerTexture) {
    printf("Failed to load texture: %s\n", SDL_GetError());
    }

    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);
    Player player = init_player(window_width, window_height);

    movement(window, renderer, &player, window_width, window_height, playerTexture,background_texture);

    // SDL_Quit();
}