#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>


#define PLAYER_SPEED 350

typedef enum player_state{IDLE,Playing}PlayerState;

typedef struct position{
    float y;
    float x;
}Position;

typedef struct velocity{
    float speed;
    float dx;
    float dy;
    float acceleration;
    float max_speed;
}Velocity;

typedef struct Controls{
    SDL_Scancode up;
    SDL_Scancode down;
    SDL_Scancode right;
    SDL_Scancode left;
}Player_controls;
typedef struct player{
    Position player_pos;
    Velocity player_speed;
    SDL_FRect Hitbox;
    PlayerState state;
    Player_controls player_controls;
}Player;

Player init_player(int window_width,int window_height);
Player init_player(int window_width, int window_height);
void move_player(int window_width, int window_height, Player *player, float dt);
void renderPlayer(SDL_Renderer *renderer, Player *player);
void movement(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *backgroundTexture, Player *player, int window_width, int window_height);
SDL_Texture *loading_img(SDL_Renderer *renderer, const char *path);

