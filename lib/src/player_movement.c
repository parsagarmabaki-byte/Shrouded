#define SDL_MAIN_HANDLED
#include <SDL2/SDL_image.h>
#include "player_movement.h"
#include "game_map.h"
#include <math.h>


void apply_movement(float *x, float *y, float w, float h, int up, int down, int left, int right, float dt)
{
    float dx = 0, dy = 0;
    if (up)    
    {
        dy -= 1;
    }
    if (down)  
    {
        dy += 1;
    }
    if (left)  
    {
        dx -= 1;
    }
    if (right) 
    {
        dx += 1;
    }

    if (dx != 0 || dy != 0) 
    {
        float len = sqrtf(dx * dx + dy * dy);
        dx /= len;
        dy /= len;
    }

    *x += dx * PLAYER_SPEED * dt; // SERVER TICK RATE
    *y += dy * PLAYER_SPEED * dt;

    if (*x < 0) 
    {
        *x = 0;
    }
    if (*y < 0) 
    {
        *y = 0;
    }
    if (*x + w > GAME_MAP_WIDTH) 
    {
        *x = GAME_MAP_WIDTH - w;
    }
    if (*y + h > GAME_MAP_HEIGHT) 
    {
        *y = GAME_MAP_HEIGHT - h;
    }
}



Player init_player(int window_width, int window_height)
{
    /*
    Player player = {{window_width / 2 - 45/2, window_height / 2 - 70/2, PLAYER_SIZE, PLAYER_SIZE}, Playing, {SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_A}};
    ANVÄNDS EJ LÄNGRE
    */
    Player player = {{window_width / 2 - 45/2, window_height / 2 - 70/2, PLAYER_SIZE, PLAYER_SIZE}};
    player.direction = DIR_DOWN;
    player.current_frame = 0;
    player.animation_timer = 0.0f;
    return player;

}

void update_map(SDL_Renderer *renderer, SDL_Texture *Game_map, Player *player, SDL_Texture *player_sprite, Camera *cam)
{
    render_map(renderer, Game_map, cam);
    renderPlayer(renderer, player, player_sprite, cam);
}

void renderPlayer(SDL_Renderer *renderer, Player *player, SDL_Texture *texture, Camera *cam)
{
    SDL_Rect src;
    SDL_Rect dst;

    // --- Pick frame from sprite sheet ---
    src.x = player->current_frame * FRAME_SIZE;
    src.y = player->direction * FRAME_SIZE;
    src.w = FRAME_SIZE;
    src.h = FRAME_SIZE;

    // --- Where to draw on screen ---
    dst.x = (int)player->Hitbox.x;
    dst.y = (int)player->Hitbox.y;

    // Rita spelaren med camera-offset
    dst.x = (int)(player->Hitbox.x - cam->x);
    dst.y = (int)(player->Hitbox.y - cam->y);
    dst.w = (int)player->Hitbox.w;
    dst.h = (int)player->Hitbox.h;

    SDL_RenderCopy(renderer, texture, &src, &dst);
}

/*
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

    if (player->Hitbox.x < 0) player->Hitbox.x = 0;
    if (player->Hitbox.y < 0) player->Hitbox.y = 0;
    if (player->Hitbox.x + player->Hitbox.w > GAME_MAP_WIDTH)
        player->Hitbox.x = GAME_MAP_WIDTH - player->Hitbox.w;
    if (player->Hitbox.y + player->Hitbox.h > GAME_MAP_HEIGHT)
        player->Hitbox.y = GAME_MAP_HEIGHT - player->Hitbox.h;

    return moving;
}
*/

/* void movement(SDL_Window *window, SDL_Renderer *renderer, Player *player, int window_width, int window_height, SDL_Texture *player_sprite, SDL_Texture *GameMap)
{
    SDL_Event event;
    InputState input;
    bool running = true;
    Uint64 last = SDL_GetPerformanceCounter();

    Camera cam = {0, 0, window_width, window_height};

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

                if (player->current_frame >= 10)  // we have ten frames
                {
                    player->current_frame = 0;
                }
            }
        }
        else
        {
            player->current_frame = 2; // reset to idle frame when not moving
        }
        
        camera_follow(&cam, player->Hitbox.x, player->Hitbox.y, player->Hitbox.w, player->Hitbox.h);
        update_map(renderer, GameMap, player, player_sprite, &cam);
    }
}
*/

/*void read_input(Player player,InputState *input) ANVÄNDS INTE LÄNGRE
{
    const Uint8 *key = SDL_GetKeyboardState(NULL);

    input->up = key[player.controls.up];
    input->down = key[player.controls.down];
    input->left = key[player.controls.left];
    input->right = key[player.controls.right];
}
*/
