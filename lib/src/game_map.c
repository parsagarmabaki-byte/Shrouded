#include "game_map.h"

SDL_Texture *loading_img(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *surface = IMG_Load(path);
    if (!surface)
    {
        printf("IMG_Load failed: %s\n", IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture)
    {
        printf("SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
        return NULL;
    }

    return texture;
}

void render_map(SDL_Renderer *renderer, SDL_Texture *background_img, int window_width, int window_height)
{
    SDL_Rect picturesize = {
        (window_width - GAME_MAP_WIDTH) / 2,
        (window_height / Game_MAP_HEIGHT) / 2,
        GAME_MAP_WIDTH, Game_MAP_HEIGHT};
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, background_img, NULL, &picturesize);
}


