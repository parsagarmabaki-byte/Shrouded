#define SDL_MAIN_HANDLED
#include <SDL2/SDL_image.h>
#include "player_movement.h"
#include "game_map.h"
#include <math.h>

bool move_player(Player *player,InputState input,float dt)
{
    bool moving = false;
    float dx = 0;
    float dy = 0;

    if (input.up)
    {
        dy -= 1;
        player->direction = DIR_UP;
        moving = true;
    }
    if (input.down)
    {
        dy += 1;
        player->direction = DIR_DOWN;
        moving = true;
    }
    if (input.left)
    {
        dx -= 1;
        player->direction = DIR_LEFT;
        moving = true;
    }
    if (input.right)
    {
        dx += 1;
        player->direction = DIR_RIGHT;
        moving = true;
    }

    if (dx != 0 || dy != 0)
    {
        float length = sqrtf(dx * dx + dy * dy);
        dx /= length;
        dy /= length;
    }

    player->Hitbox.x += dx * PLAYER_SPEED * dt;
    player->Hitbox.y += dy * PLAYER_SPEED * dt;

    return moving;
}

void movement(SDL_Window *window, SDL_Renderer *renderer, Player *player, int window_width, int window_height, SDL_Texture *player_sprite,SDL_Texture *GameMap)
{
    SDL_Event event;
    InputState input;
    bool running = true;
    Uint64 last = SDL_GetPerformanceCounter();

    while (running)
    {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency();
        last = now;

        while (SDL_PollEvent(&event))
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    running = false;
                }
            }

        read_input(*player,&input);
        bool moving = move_player(player,input,dt);
        
        if(moving)
        {
            player->animation_timer += dt;
            if (player->animation_timer > 0.1f)
            {
                player->current_frame++;
                player->animation_timer = 0;

                if (player->current_frame >= 4)  // assume 4 frames
                {
                    player->current_frame = 0;
                }
            }
        }
        else
        {
            player->current_frame = 2; // reset to idle frame when not moving
        }
        
        update_map(renderer,GameMap,player,player_sprite,window_width,window_height);
    }
}

void read_input(Player player,InputState *input)
{
    const Uint8 *key = SDL_GetKeyboardState(NULL);

    input->up = key[player.controls.up];
    input->down = key[player.controls.down];
    input->left = key[player.controls.left];
    input->right = key[player.controls.right];
}

Player init_player(int window_width, int window_height)
{
    Player player = {{window_width / 2 - 45/2, window_height / 2 - 70/2, 70, 70}, Playing, {SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_A}};
    player.direction = DIR_DOWN;
    player.current_frame = 0;
    player.animation_timer = 0.0f;
    return player;
}

void update_map(SDL_Renderer *renderer, SDL_Texture *Game_map, Player *player ,SDL_Texture *player_sprite, int window_width, int window_height)
{
    render_map(renderer,Game_map,window_width,window_height);
    renderPlayer(renderer, player, player_sprite);
}

void renderPlayer(SDL_Renderer *renderer, Player *player, SDL_Texture *texture)
{
    int frame_width = 128; 
    int frame_height = 128;

    SDL_Rect src;
    SDL_Rect dst;

    // --- Pick frame from sprite sheet ---
    src.x = player->current_frame * frame_width;
    src.y = player->direction * frame_height;
    src.w = frame_width;
    src.h = frame_height;

    // --- Where to draw on screen ---
    dst.x = (int)player->Hitbox.x;
    dst.y = (int)player->Hitbox.y;
    dst.w = (int)player->Hitbox.w;
    dst.h = (int)player->Hitbox.h;

    SDL_RenderCopy(renderer, texture, &src, &dst);

    SDL_RenderPresent(renderer);
}