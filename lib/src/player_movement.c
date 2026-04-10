#define SDL_MAIN_HANDLED
#include "player_movement.h"

Player init_player(int window_width, int window_height)
{
    Player player = {
        .player_pos = {window_width / 2.0f, window_height / 2.0f},
        .player_speed = {0,0,0,0,0},
        .Hitbox = {window_width / 2.0f, window_height / 2.0f, 45.0f, 70.0f},
        .state = Playing,
        .player_controls = {
            SDL_SCANCODE_W,
            SDL_SCANCODE_S,
            SDL_SCANCODE_D,
            SDL_SCANCODE_A
        }
    };

    return player;
}

void move_player(int window_width, int window_height, Player *player, float dt)
{
    const Uint8 *key = SDL_GetKeyboardState(NULL);

    if (key[player->player_controls.up])
        player->Hitbox.y -= PLAYER_SPEED * dt;

    if (key[player->player_controls.down])
        player->Hitbox.y += PLAYER_SPEED * dt;

    if (key[player->player_controls.left])
        player->Hitbox.x -= PLAYER_SPEED * dt;

    if (key[player->player_controls.right])
        player->Hitbox.x += PLAYER_SPEED * dt;

    // Keep player inside screen
    if (player->Hitbox.x < 0)
        player->Hitbox.x = 0;
    if (player->Hitbox.y < 0)
        player->Hitbox.y = 0;
    if (player->Hitbox.x + player->Hitbox.w > window_width)
        player->Hitbox.x = window_width - player->Hitbox.w;
    if (player->Hitbox.y + player->Hitbox.h > window_height)
        player->Hitbox.y = window_height - player->Hitbox.h;
}

void renderPlayer(SDL_Renderer *renderer, Player *player)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderFillRectF(renderer, &player->Hitbox);
}

void movement(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *backgroundTexture, Player *player, int window_width, int window_height)
{
    SDL_Event event;
    bool running = true;
    Uint64 last = SDL_GetPerformanceCounter();

    while (running)
    {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency();
        last = now;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;

            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                running = false;
        }

        move_player(window_width, window_height, player, dt);
        SDL_Rect destRect = {(window_width - 1536)/2, (window_height- 1024)/2, 1536, 1024}; //map image size

        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, backgroundTexture, NULL, &destRect);
        renderPlayer(renderer, player);
        SDL_RenderPresent(renderer);
    }
}