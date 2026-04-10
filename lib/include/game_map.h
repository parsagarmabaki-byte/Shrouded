#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>

#define GAME_MAP_WIDTH 1536
#define Game_MAP_HEIGHT 1024

SDL_Texture *loading_img(SDL_Renderer *renderer, const char *path);

