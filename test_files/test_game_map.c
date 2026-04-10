#define SDL_MAIN_HANDLED
#include "game_map.h"

int main(void)
{
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    int window_width, window_height;

    SDL_Window *window = SDL_CreateWindow(
        "Game",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        0, 0,
        SDL_WINDOW_FULLSCREEN_DESKTOP);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    // zoomed out compared to 1280x720
    // SDL_RenderSetLogicalSize(renderer, 1600, 900);
    SDL_GetWindowSize(window, &window_width, &window_height);
    SDL_Texture *background_texture = loading_img(renderer, "assets/images/Game_map.png");
    SDL_Rect picturesize = {
        (window_width - GAME_MAP_WIDTH) / 2,
        (window_height / Game_MAP_HEIGHT) / 2,
        GAME_MAP_WIDTH, Game_MAP_HEIGHT};
    SDL_RenderCopy(renderer, background_texture, NULL,&picturesize);
    SDL_RenderPresent(renderer);
    SDL_Delay(1000);
    return 0;
}