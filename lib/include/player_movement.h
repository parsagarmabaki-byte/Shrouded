#ifndef PLAYER_MOVEMENT_H
#define PLAYER_MOVEMENT_H

#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>
#include "game_map.h"
#include "network_data.h"

typedef enum player_state{IDLE,Playing}PlayerState;

typedef enum {
    DIR_LEFT,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_UP
} Direction;

typedef struct Controls{
    SDL_Scancode up;
    SDL_Scancode down;
    SDL_Scancode right;
    SDL_Scancode left;
}Player_controls;

typedef enum
{
    PLAYER_RED,
    PLAYER_BLUE,
    PLAYER_GREEN,
    PLAYER_YELLOW,
    PLAYER_PURPLE,
    PLAYER_BLACK
} PlayerColor;

typedef struct
{
    bool up;
    bool down;
    bool left;
    bool right;
} InputState;


typedef struct player{
    SDL_FRect Hitbox;
    PlayerState State;

    Player_controls controls;
    Direction direction;

    PlayerColor SkinColor;
    int current_frame;
    float animation_timer;
}Player;

Player init_player(int window_width,int window_height);
void movement(SDL_Window *window, SDL_Renderer *renderer, Player *player, int window_width, int window_height, SDL_Texture *texture,SDL_Texture *background_texture);
void renderPlayer(SDL_Renderer *renderer, Player *player, SDL_Texture *texture, Camera *cam);
void update_map(SDL_Renderer *renderer, SDL_Texture *Game_map, Player *player, SDL_Texture *player_sprite, Camera *cam);
void read_input(Player player,InputState *input);
void apply_movement(float *x, float *y, float w, float h, int up, int down, int left, int right, float dt);
#endif