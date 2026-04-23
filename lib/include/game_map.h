#ifndef GAME_MAP_H
#define GAME_MAP_H

#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>

#define GAME_MAP_WIDTH 2536
#define GAME_MAP_HEIGHT 2024
#define LOGICAL_SCREEN_WIDTH 1280
#define LOGICAL_SCREEN_HEIGHT 720
#define FRAME_SIZE 256
#define PLAYER_SLOTS 6

typedef struct
{
    SDL_Texture *skins[PLAYER_SLOTS];
    SDL_Texture *dead_skins[PLAYER_SLOTS];
    SDL_Texture *map_texture;
    SDL_Texture *vignette_img;
    SDL_Texture *killer_img;
    SDL_Texture *innocent_img;
    SDL_Texture *role_art_img;
    SDL_Texture *kill_button_active;
    SDL_Texture *kill_button_deactive;
} GameAssets;

typedef struct {
    float x, y;
    int screen_w, screen_h;
} Camera;

GameAssets load_assets(SDL_Renderer *renderer);
SDL_Texture *loading_img(SDL_Renderer *renderer, const char *path);
void render_map(SDL_Renderer *renderer, SDL_Texture *background_img, Camera *cam);
void camera_follow(Camera *cam, float player_x, float player_y, int player_w, int player_h);
#endif
 
