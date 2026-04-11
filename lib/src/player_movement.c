#define SDL_MAIN_HANDLED
#include <SDL2/SDL_image.h>
#include "player_movement.h"
#include "game_map.h"
#include <math.h>

bool move_player(Player *player, float dt)
{
    const Uint8 *key = SDL_GetKeyboardState(NULL);

    bool moving = false;
    float dx = 0;
    float dy = 0;

    if (key[player->player_controls.up])
    {
        dy -= 1;
        player->direction = DIR_UP;
        moving = true;
    }
    if (key[player->player_controls.down])
    {
        dy += 1;
        player->direction = DIR_DOWN;
        moving = true;
    }
    if (key[player->player_controls.left])
    {
        dx -= 1;
        player->direction = DIR_LEFT;
        moving = true;
    }
    if (key[player->player_controls.right])
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

    // Clamp spelaren inom kartans gränser
    if (player->Hitbox.x < 0) player->Hitbox.x = 0;
    if (player->Hitbox.y < 0) player->Hitbox.y = 0;
    if (player->Hitbox.x + player->Hitbox.w > GAME_MAP_WIDTH)
        player->Hitbox.x = GAME_MAP_WIDTH - player->Hitbox.w;
    if (player->Hitbox.y + player->Hitbox.h > GAME_MAP_HEIGHT)
        player->Hitbox.y = GAME_MAP_HEIGHT - player->Hitbox.h;

    return moving;
}

void movement(SDL_Window *window, SDL_Renderer *renderer, Player *player, int window_width, int window_height, SDL_Texture *player_sprite, SDL_Texture *GameMap)
{
    SDL_Event event;
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
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                    running = false;

        bool moving = move_player(player, dt);

        if (moving)
        {
            player->animation_timer += dt;
            if (player->animation_timer > 0.1f)
            {
                player->current_frame++;
                player->animation_timer = 0;
                if (player->current_frame >= 4)
                    player->current_frame = 0;
            }
        }
        else
        {
            player->current_frame = 2;
        }

        camera_follow(&cam, player->Hitbox.x, player->Hitbox.y, player->Hitbox.w, player->Hitbox.h);
        update_map(renderer, GameMap, player, player_sprite, &cam);
    }
}

Player init_player(int window_width, int window_height)
{
    Player player = {
        {window_width / 2, window_height / 2},
        {0, 0, 0, 0, 0},
        {window_width / 2 - 45 / 2, window_height / 2 - 70 / 2, 100, 100},
        Playing,
        {SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_A}
    };
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
    int frame_width = 128;
    int frame_height = 128;

    SDL_Rect src;
    SDL_Rect dst;

    src.x = player->current_frame * frame_width;
    src.y = player->direction * frame_height;
    src.w = frame_width;
    src.h = frame_height;

    // Rita spelaren med camera-offset
    dst.x = (int)(player->Hitbox.x - cam->x);
    dst.y = (int)(player->Hitbox.y - cam->y);
    dst.w = (int)player->Hitbox.w;
    dst.h = (int)player->Hitbox.h;

    SDL_RenderCopy(renderer, texture, &src, &dst);
    //SDL_RenderPresent(renderer);
}