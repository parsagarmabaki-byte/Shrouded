#ifndef PLAYER_MOVEMENT_H
#define PLAYER_MOVEMENT_H

#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SDL2/SDL_image.h>
#include "game_map.h"
#include "network_data.h"


typedef struct{
    SDL_FRect Hitbox;
    Direction direction;
    int current_frame;
    float animation_timer;
    bool kill_cooldown_active;
    Uint32 kill_cooldown_end;
} Player;

Player *player_create(gameState *state, int local_id);
void player_destroy(Player *p);

//-----
Player init_player(gameState state, int local_id);
void renderPlayer(SDL_Renderer *renderer, Player *player, SDL_Texture *texture, Camera *cam);
void update_map(SDL_Renderer *renderer, SDL_Texture *Game_map, Player *player, SDL_Texture *player_sprite, Camera *cam);
void apply_movement(float *x, float *y, clientInput input, float dt);
void compare_server_position(gameState state, Player *player,int local_id);


#endif