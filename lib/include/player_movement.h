#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>


#define PLAYER_SPEED 140

typedef enum player_state{IDLE,Playing}PlayerState;

typedef enum {
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
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
void renderPlayer(SDL_Renderer *renderer, Player *player, SDL_Texture *texture);
void update_map(SDL_Renderer *renderer, SDL_Texture *Game_map, Player *player ,SDL_Texture *player_sprite, int window_width, int window_height);
void read_input(Player player,InputState *input);


