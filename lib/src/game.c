#include "game.h"
#include "network.h"
#include "game_map.h"
#include "player_movement.h"
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

// Must match PLAYER_SPEED in server.c for prediction to stay in sync
#define PLAYER_SPEED 200

// Reads keyboard state and sends the local player's input to the server every frame.
// The server uses this to update the authoritative player position.
void sendInput(Client *client, gameState *state)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    clientInput input = {0};
    input.type      = MSG_CLIENT_INPUT;
    input.player_id = state->local_player_id;
    input.up        = keys[SDL_SCANCODE_W];
    input.down      = keys[SDL_SCANCODE_S];
    input.left      = keys[SDL_SCANCODE_A];
    input.right     = keys[SDL_SCANCODE_D];
    send_client_input(client->socket, client->serverAddr, &input);
}

// Main game loop. Runs after the lobby phase when the server signals GAME_RUNNING.
// Handles input, client-side prediction, receiving server state, and rendering.
void runGame(Client *client, waitForPlayers *lobby, gameState *state)
{
    SDL_Renderer *renderer = lobby->renderer;

    // Use the actual window size as the logical render resolution
    int window_width, window_height;
    SDL_GetWindowSize(lobby->window, &window_width, &window_height);
    SDL_RenderSetLogicalSize(renderer, window_width, window_height);

    // Load assets
    SDL_Texture *mapTexture = loading_img(renderer, "assets/images/Game_map.png");
    if (!mapTexture) { printf("Failed to load map\n"); return; }

    SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "assets/sprites/charspritesv2.png");
    if (!playerTexture) { printf("Failed to load player sprite\n"); return; }

    // Initialize the local player at the spawn position received from the server
    int local_id = state->local_player_id;
    Player player = init_player(window_width, window_height);
    player.Hitbox.x = state->players[local_id].x;
    player.Hitbox.y = state->players[local_id].y;

    // Camera starts at origin — camera_follow() centers it on the player each frame
    Camera cam = {0, 0, window_width, window_height};

    // Ensure the game window has focus so keyboard input is captured
    SDL_RaiseWindow(lobby->window);
    SDL_SetWindowInputFocus(lobby->window);

    SDL_Event event;
    bool running = true;
    Uint64 last = SDL_GetPerformanceCounter();

    while (running)
    {
        // Delta time — time since last frame in seconds
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency();
        last = now;

        // Handle window and keyboard events
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            {
                send_leave(client->socket, client->serverAddr);
                running = false;
            }
        }

        // Send this frame's input to the server
        sendInput(client, state);

        // Receive the latest game state from the server (non-blocking)
        receive_game_state(client->socket, client->recievepacket, state);

        // --- Client-side prediction ---
        // Move the local player immediately based on input without waiting for
        // the server response. This removes the one-frame input delay.
        // The server remains authoritative — other players use server positions.
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        float dx = 0, dy = 0;
        if (keys[SDL_SCANCODE_W]) { dy -= 1; player.direction = DIR_UP; }
        if (keys[SDL_SCANCODE_S]) { dy += 1; player.direction = DIR_DOWN; }
        if (keys[SDL_SCANCODE_A]) { dx -= 1; player.direction = DIR_LEFT; }
        if (keys[SDL_SCANCODE_D]) { dx += 1; player.direction = DIR_RIGHT; }

        // Normalize diagonal movement so speed is consistent in all directions
        if (dx != 0 && dy != 0) { dx *= 0.7071f; dy *= 0.7071f; }

        player.Hitbox.x += dx * PLAYER_SPEED * dt;
        player.Hitbox.y += dy * PLAYER_SPEED * dt;

        // --- Animation ---
        // Advance animation frames while moving, reset to idle frame when stopped
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
            player.current_frame = 2; // idle frame
        }

        // Move the camera to keep the local player centered on screen
        camera_follow(&cam, player.Hitbox.x, player.Hitbox.y, player.Hitbox.w, player.Hitbox.h);

        // Draw the map with the camera offset
        render_map(renderer, mapTexture, &cam);

        // Draw all active players
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (state->players[i].active)
            {
                Player p = player;

                if (i == local_id)
                {
                    // Local player — use predicted position and animation
                    p.Hitbox.x     = player.Hitbox.x;
                    p.Hitbox.y     = player.Hitbox.y;
                    p.current_frame = player.current_frame;
                    p.direction    = player.direction;
                }
                else
                {
                    // Other players — use authoritative server position, idle animation
                    p.Hitbox.x     = state->players[i].x;
                    p.Hitbox.y     = state->players[i].y;
                    p.current_frame = 2;
                    p.direction    = DIR_DOWN;
                }

                renderPlayer(renderer, &p, playerTexture, &cam);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // cap at ~60fps
    }

    SDL_DestroyTexture(mapTexture);
    SDL_DestroyTexture(playerTexture);
}