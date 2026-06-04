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

    SDL_Texture *kill_button_active;
    SDL_Texture *kill_button_deactive;

    SDL_Texture *report_button_deactive;
    SDL_Texture *report_button_active;

    SDL_Texture *emergency_button_view;
    SDL_Texture *emergency_button_hover;
    SDL_Texture *emergency_meeting_info;
    SDL_Texture *dead_body_reported_info;

    SDL_Texture *emergency_meeting_alive;
    SDL_Texture *emergency_meeting_submit;
    SDL_Texture *emergency_meeting_skip;
    SDL_Texture *emergency_meeting_dead;

    SDL_Texture *emergency_meeting_icon;

    SDL_Texture *players_alive_banner[PLAYER_SLOTS];
    SDL_Texture *players_alive_banner_hover[PLAYER_SLOTS];
    SDL_Texture *players_dead_banner[PLAYER_SLOTS];

    SDL_Texture *players_kicked_out[PLAYER_SLOTS];
    SDL_Texture *players_voting_result_alive[PLAYER_SLOTS];
    SDL_Texture *players_voting_result_dead[PLAYER_SLOTS];
    SDL_Texture *skip_vote_banner;
    SDL_Texture *no_one_eliminated;
    SDL_Texture *killer_win_screens[PLAYER_SLOTS];
    SDL_Texture *crewmates_win_screens[PLAYER_SLOTS];

    // Pausmeny
    SDL_Texture *pause_bg;
    SDL_Texture *pause_resume;
    SDL_Texture *pause_exit;

    SDL_Texture *task_indicator;
} GameAssets;

typedef struct
{
    SDL_Texture *killer_img;
    SDL_Texture *innocent_img;
    SDL_Texture *role_art_img;
} Game_Show_Role_asset;

typedef struct
{
    float x, y;
    int screen_w, screen_h;
} Camera;

GameAssets load_assets(SDL_Renderer *renderer);
SDL_Texture *loading_img(SDL_Renderer *renderer, const char *path);
void render_map(SDL_Renderer *renderer, SDL_Texture *background_img, Camera *cam);
void camera_follow(Camera *cam, float player_x, float player_y, int player_w, int player_h);
Game_Show_Role_asset load_show_role_assets(SDL_Renderer *renderer);
void destroy_assets(GameAssets *asset, SDL_Texture *player_role);

#endif
