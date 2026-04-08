#define SDL_MAIN_HANDLED
#include "player_movement.h"

int main()
{
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    window = SDL_CreateWindow("Test window", 0, 0, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    renderer = SDL_CreateRenderer(window, -1, 0);

    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);
    Player player = init_player(window_width, window_height);

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderFillRectF(renderer, &(player.Hitbox));
    SDL_RenderPresent(renderer);
    // SDL_Delay(1000);

    movement(window, renderer, &player, window_width, window_height);

    // SDL_Quit();
}

void move_player(int window_width, int window_height, Player *player, float dt)
{
    const Uint8 *key = SDL_GetKeyboardState(NULL);
    if (key[player->player_controls.up])
    {
        //printf("%.5f\n%d\n", (PLAYER_SPEED * dt), PLAYER_SPEED);
        player->Hitbox.y -= PLAYER_SPEED * dt;
        player->direction = DIR_UP;
    }
    if (key[player->player_controls.down])
    {
        player->Hitbox.y += PLAYER_SPEED * dt;
        player->direction = DIR_DOWN;
    }
    if (key[player->player_controls.left])
    {
        player->Hitbox.x -= PLAYER_SPEED * dt;
        player->direction = DIR_LEFT;
    }
    if (key[player->player_controls.right])
    {
        player->Hitbox.x += PLAYER_SPEED * dt;
        player->direction = DIR_RIGHT;
    }
}

void renderPlayer(SDL_Window *window, SDL_Renderer *renderer, Player player)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderFillRectF(renderer, &(player.Hitbox));
    SDL_RenderPresent(renderer);
}

void movement(SDL_Window *window, SDL_Renderer *renderer, Player *player, int window_with, int window_height)
{
    SDL_Event event;
    bool running = true;
    Uint64 last = SDL_GetPerformanceCounter();
    while (running)
    {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / SDL_GetPerformanceFrequency();
        last = now;

        while (SDL_PollEvent(&event))
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    running = false;
                }
            }
        move_player(window_with, window_height, player, dt);
        player->animation_timer += dt;
        if (player->animation_timer > 0.1f)
        {
            player->current_frame++;
            player->animation_timer = 0;

            if (player->current_frame >= 4)  // assume 4 frames
            {
                player->current_frame = 0;
            }
            printf("Frame: %d\n", player->current_frame);
        }
        renderPlayer(window, renderer, (*player));
    }
}

Player init_player(int window_width, int window_height)
{
    Player player = {{window_width / 2, window_height / 2}, {0, 0, 0, 0, 0}, {window_width / 2 - 45/2, window_height / 2 - 70/2, 45, 70}, Playing, {SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_A}};
    player.direction = DIR_DOWN;
    //printf("Direction: %d\n", player.direction);
    player.current_frame = 0;
    player.animation_timer = 0.0f;
    return player;
}
