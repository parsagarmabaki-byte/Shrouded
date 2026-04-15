#include "game.h"
#include "network.h"
#include "game_map.h"
#include "player_movement.h"
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

// Must match PLAYER_SPEED in server.c for prediction to stay in sync
// Reads keyboard state and sends the local player's input to the server every frame.
// The server uses this to update the authoritative player position.
void sendInput(Client *client, gameState *state)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    clientInput input = {0};
    input.type      = MSG_CLIENT_INPUT;
    input.player_id = state->local_player_id;
    input.up        = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    input.down      = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    input.left      = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    input.right     = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    send_client_input(client->socket, client->serverAddr, &input);
}

// Main game loop. Runs after the lobby phase when the server signals GAME_RUNNING.
// Handles input, client-side prediction, receiving server state, and rendering.
void runGame(Client *client, waitForPlayers *lobby, gameState *state)
{
    SDL_Renderer *renderer = lobby->renderer;

    SDL_RenderSetLogicalSize(renderer, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);

    GameAssets assets = load_assets(renderer);
    if (!assets.map_texture) { printf("Failed to load map\n"); return; }
    if (!assets.skins[0]) { printf("Failed to load player sprite\n"); return; }

    // Initialize the local player at the spawn position received from the server
    int local_id = state->local_player_id;
    Player player = init_player(LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    player.Hitbox.x = state->players[local_id].x;
    player.Hitbox.y = state->players[local_id].y;

    // Camera starts at origin — camera_follow() centers it on the player each frame
    Camera cam = {0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT};

    // Ensure the game window has focus so keyboard input is captured
    SDL_RaiseWindow(lobby->window);
    SDL_SetWindowInputFocus(lobby->window);

    SDL_Event event;
    bool running = true;
    Uint64 last = SDL_GetPerformanceCounter();




    //TEST för att få bort drift
    float accumulator = 0.0f;

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

        /*
        När ett nytt serverpaket kommer, snäpp tillbaka den lokala
        spelaren till serverns auktoritativa position. Det hindrar
        prediction från att drifta över tid, men känns fortfarande
        responsivt mellan paket (prediction kör fritt tills nästa korrigering).
        */
        if (receive_game_state(client->socket, client->recievepacket, state) == 0)
        {
            player.Hitbox.x = state->players[local_id].x;
            player.Hitbox.y = state->players[local_id].y;
        }

        accumulator += dt;

        int up = 0, down = 0, left = 0, right = 0;

        while (accumulator >= SERVER_TICK_INTERVAL) //SERVER TICK ÄR 0.016f
        {
            const Uint8 *keys = SDL_GetKeyboardState(NULL);
            up    = keys[SDL_SCANCODE_W];
            down  = keys[SDL_SCANCODE_S];
            left  = keys[SDL_SCANCODE_A];
            right = keys[SDL_SCANCODE_D];

            if (up)    {player.direction = DIR_UP;}
            if (down)  {player.direction = DIR_DOWN;}
            if (left)  {player.direction = DIR_LEFT;}
            if (right) {player.direction = DIR_RIGHT;}

            apply_movement(&player.Hitbox.x, &player.Hitbox.y, player.Hitbox.w, player.Hitbox.h, up, down, left, right, SERVER_TICK_INTERVAL);

            accumulator -= SERVER_TICK_INTERVAL;
        }
       

        // --- Animation ---------- ---------
        // Advance animation frames while moving, reset to idle frame when stopped
        bool moving = up || down || left || right;

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
        render_map(renderer, assets.map_texture, &cam);

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

                renderPlayer(renderer, &p, assets.skins[0], &cam);
            }
        }

        if (assets.vignette_img)
            SDL_RenderCopy(renderer, assets.vignette_img, NULL, NULL);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(assets.map_texture);
    SDL_DestroyTexture(assets.vignette_img);
    for (int i = 0; i < PLAYER_SLOTS; i++)
        SDL_DestroyTexture(assets.skins[i]);
}
