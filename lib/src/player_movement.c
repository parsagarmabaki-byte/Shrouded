#define SDL_MAIN_HANDLED
#include "player_movement.h"
int main(void)
{
    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        printf("IMG_Init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Test window",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    if (!window)
    {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer)
    {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    Player player = init_player(window_width, window_height);

    SDL_Texture *backgroundTexture = loading_img(renderer, "assets/images/Game_map.png");
    if (!backgroundTexture)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    movement(window, renderer, backgroundTexture, &player, window_width, window_height);

    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}

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

SDL_Texture *loading_img(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *surface = IMG_Load(path);
    if (!surface)
    {
        printf("IMG_Load failed: %s\n", IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture)
    {
        printf("SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
        return NULL;
    }

    return texture;
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

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
        renderPlayer(renderer, player);
        SDL_RenderPresent(renderer);
    }
}