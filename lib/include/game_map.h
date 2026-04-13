#ifndef GAME_MAP_H_INCLUDED
#define GAME_MAP_H_INCLUDED
 
#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>
 
#define GAME_MAP_WIDTH 3072
#define GAME_MAP_HEIGHT 2048
 
typedef struct {
    float x, y;
    int screen_w, screen_h;
} Camera;
 
SDL_Texture *loading_img(SDL_Renderer *renderer, const char *path);
void render_map(SDL_Renderer *renderer, SDL_Texture *background_img, Camera *cam);
void camera_follow(Camera *cam, float player_x, float player_y, float player_w, float player_h);
 
#endif
 