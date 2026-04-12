#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>

#define GAME_MAP_WIDTH 1536
#define GAME_MAP_HEIGHT 1024
#define FRAME_SIZE 256
#define PLAYER_SLOTS 4

typedef struct
{
    SDL_Texture *skins[PLAYER_SLOTS];
    SDL_Texture *map_texture;
} GameAssets;

SDL_Texture *loading_img(SDL_Renderer *renderer, const char *path);
void render_map(SDL_Renderer *renderer, SDL_Texture *background_img, int window_width, int window_height);
GameAssets load_assets(SDL_Renderer *renderer);

