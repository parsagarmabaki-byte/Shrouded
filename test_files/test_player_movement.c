#define SDL_MAIN_HANDLED
#include "player_movement.h"
#include "game_map.h"

int main(void)
{
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window *window = SDL_CreateWindow("Test window", 0, 0, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, 1536, 1024);

    int window_width = 1536;
    int window_height = 1024;

    SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "assets/sprites/charspritesv2.png");
    SDL_Texture *background_texture = loading_img(renderer, "assets/images/Game_map.png");

    if (!playerTexture)
        printf("Failed to load player texture: %s\n", SDL_GetError());
    if (!background_texture)
        printf("Failed to load map texture: %s\n", IMG_GetError());

    Player player = init_player(window_width, window_height);

    movement(window, renderer, &player, window_width, window_height, playerTexture, background_texture);
}