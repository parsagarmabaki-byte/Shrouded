#define SDL_MAIN_HANDLED
#include "game_map.h"

int main(void)
{
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    int window_width, window_height;
    
    SDL_Window* window = SDL_CreateWindow(
    "Game",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    0,0,
    SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    // zoomed out compared to 1280x720
    SDL_RenderSetLogicalSize(renderer, 1600, 900);
    SDL_GetWindowSize(window,&window_width,&window_height);
    loading_img(window,renderer,window_width,window_height);   
    return 0;
}

void loading_img(SDL_Window *window, SDL_Renderer *renderer, int window_width, int window_height)
{
    SDL_Surface *surface = IMG_Load("assets/images/Game_map.png");
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    SDL_Rect destRect = {150, 50, window_width, window_height};

    SDL_RenderCopy(renderer, texture, NULL, &destRect);
    SDL_RenderPresent(renderer);
}

