#include "game.h"
#include "network.h"
#include "game_map.h"
#include "player_movement.h"
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define PLAYER_SPEED 200

void sendInput(Client *client, gameState *state)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    clientInput input = {0};
    input.type = MSG_CLIENT_INPUT;
    input.player_id = state->local_player_id;
    input.up    = keys[SDL_SCANCODE_W];
    input.down  = keys[SDL_SCANCODE_S];
    input.left  = keys[SDL_SCANCODE_A];
    input.right = keys[SDL_SCANCODE_D];
    send_client_input(client->socket, client->serverAddr, &input);
}

void runGame(Client *client, waitForPlayers *lobby, gameState *state)
{
    SDL_Renderer *renderer = lobby->renderer;

    int window_width, window_height;
    SDL_GetWindowSize(lobby->window, &window_width, &window_height);
    SDL_RenderSetLogicalSize(renderer, window_width, window_height);

    SDL_Texture *mapTexture = loading_img(renderer, "assets/images/Game_map.png");
    if (!mapTexture) { printf("Failed to load map\n"); return; }

    SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "assets/sprites/charspritesv2.png");
    if (!playerTexture) { printf("Failed to load player sprite\n"); return; }

    int local_id = state->local_player_id;
    Player player = init_player(window_width, window_height);
    player.Hitbox.x = state->players[local_id].x;
    player.Hitbox.y = state->players[local_id].y;

    Camera cam = {0, 0, window_width, window_height};

    SDL_RaiseWindow(lobby->window);
    SDL_SetWindowInputFocus(lobby->window);

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
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            {
                send_leave(client->socket, client->serverAddr);
                running = false;
            }
        }

        // Skicka input och ta emot state från servern
        sendInput(client, state);
        receive_game_state(client->socket, client->recievepacket, state);

        // Client-side prediction — rör spelaren lokalt direkt
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        float dx = 0, dy = 0;
        if (keys[SDL_SCANCODE_W]) { dy -= 1; player.direction = DIR_UP; }
        if (keys[SDL_SCANCODE_S]) { dy += 1; player.direction = DIR_DOWN; }
        if (keys[SDL_SCANCODE_A]) { dx -= 1; player.direction = DIR_LEFT; }
        if (keys[SDL_SCANCODE_D]) { dx += 1; player.direction = DIR_RIGHT; }
        if (dx != 0 && dy != 0) { dx *= 0.7071f; dy *= 0.7071f; }
        player.Hitbox.x += dx * PLAYER_SPEED * dt;
        player.Hitbox.y += dy * PLAYER_SPEED * dt;

        // Animering
        bool moving = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_S] ||
                      keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_D];
        if (moving)
        {
            player.animation_timer += dt;
            if (player.animation_timer > 0.1f)
            {
                player.current_frame = (player.current_frame + 1) % 4;
                player.animation_timer = 0;
            }
        }
        else
        {
            player.current_frame = 2;
        }

        // Kamera följer lokal spelare
        camera_follow(&cam, player.Hitbox.x, player.Hitbox.y, player.Hitbox.w, player.Hitbox.h);
        render_map(renderer, mapTexture, &cam);

        // Rita alla aktiva spelare
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (state->players[i].active)
            {
                Player p = player;

                if (i == local_id)
                {
                    // Lokal spelare — använd prediction-position
                    p.Hitbox.x = player.Hitbox.x;
                    p.Hitbox.y = player.Hitbox.y;
                    p.current_frame = player.current_frame;
                    p.direction = player.direction;
                }
                else
                {
                    // Andra spelare — använd server-position
                    p.Hitbox.x = state->players[i].x;
                    p.Hitbox.y = state->players[i].y;
                    p.current_frame = 2;
                    p.direction = DIR_DOWN;
                }

                renderPlayer(renderer, &p, playerTexture, &cam);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(mapTexture);
    SDL_DestroyTexture(playerTexture);
}