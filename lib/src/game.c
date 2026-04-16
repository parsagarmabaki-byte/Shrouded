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
    input.type = MSG_CLIENT_INPUT;
    input.player_id = state->local_player_id;
    input.up = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    input.down = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    input.left = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    input.right = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    input.current_frame = state->players[state->local_player_id].current_frame;
    send_client_input(client->socket, client->serverAddr, &input);
}

clientInput read_input(void)
{
    clientInput input = {0};
    const Uint8 *key = SDL_GetKeyboardState(NULL);
    input.up = key[SDL_SCANCODE_W];
    input.down = key[SDL_SCANCODE_S];
    input.left = key[SDL_SCANCODE_A];
    input.right = key[SDL_SCANCODE_D];
    input.kill = key[SDL_SCANCODE_K];
    return input;
}

// Main game loop. Runs after the lobby phase when the server signals GAME_RUNNING.
// Handles input, client-side prediction, receiving server state, and rendering.
void runGame(Client *client, waitForPlayers *lobby, gameState *state)
{
    SDL_Renderer *renderer = lobby->renderer;

    SDL_RenderSetLogicalSize(renderer, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);

    GameAssets assets = load_assets(renderer);
    if (!assets.map_texture)
    {
        printf("Failed to load map\n");
        return;
    }
    if (!assets.skins[0])
    {
        printf("Failed to load player sprite\n");
        return;
    }

    // Initialize local player from server spawn position
    int local_id = state->local_player_id;

    // Camera starts at origin — camera_follow() centers it on the player each frame
    Camera cam = {0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT};

    // Ensure the game window has focus so keyboard input is captured
    SDL_RaiseWindow(lobby->window);
    SDL_SetWindowInputFocus(lobby->window);

    SDL_Event event;
    bool running = true;
    Uint64 last = SDL_GetPerformanceCounter();

    // TEST för att få bort drift
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
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            {
                send_leave(client->socket, client->serverAddr);
                running = false;
            }
        }

        // Send this frame's input to the server
        sendInput(client, state);

        // Receive the latest game state from the server (non-blocking)

        /*
        När ett nytt serverpaket kommer, snäpp tillbaka den lokala
        spelaren till serverns auktoritativa position. Det hindrar
        prediction från att drifta över tid, men känns fortfarande
        responsivt mellan paket (prediction kör fritt tills nästa korrigering).
        */
        // Store predicted position BEFORE server update
        float predicted_x = state->players[local_id].x;
        float predicted_y = state->players[local_id].y;

        // Receive latest game state from server (non-blocking)
        int got_state = -1;
        while (receive_game_state(client->socket, client->recievepacket, state) == 0)
        {
            got_state = 0;
        }

        // Server correction: if drift > 4px, snap to server position
        if (got_state == 0)
        {
            float server_x = state->players[local_id].x;
            float server_y = state->players[local_id].y;

            float dx = server_x - predicted_x;
            float dy = server_y - predicted_y;

            // Om skillnaden är stor: använd serverns position
            if (fabsf(dx) > 4.0f || fabsf(dy) > 4.0f)
            {
                state->players[local_id].x = server_x;
                state->players[local_id].y = server_y;
            }
            else
            {
                // Om skillnaden är liten: fortsätt på predikterad position
                state->players[local_id].x = predicted_x;
                state->players[local_id].y = predicted_y;
            }
        }

        accumulator += dt;
        clientInput player_input = read_input();

        while (accumulator >= SERVER_TICK_INTERVAL)
        {
            // Apply movement directly to state (client-side prediction)
            apply_movement(state, player_input, SERVER_TICK_INTERVAL);
            accumulator -= SERVER_TICK_INTERVAL;
        }

        // --- Animation ---------- ---------
        // Advance animation frames while moving, reset to idle frame when stopped
        bool moving = player_input.up || player_input.down || player_input.left || player_input.right;
        
        printf("DEBUG: moving=%d, up=%d, down=%d, left=%d, right=%d\n", 
               moving, player_input.up, player_input.down, player_input.left, player_input.right);

        if (moving)
        {
            if (state->players[local_id].animation_timer >= 0.1f)
            {
                state->players[local_id].current_frame++;

                if (state->players[local_id].current_frame >= 10)
                    state->players[local_id].current_frame = 0;

                state->players[local_id].animation_timer = 0.0f;
            }
        }
        else
        {
            state->players[local_id].current_frame = 2;
            state->players[local_id].animation_timer = 0.0f;
        }
        sendInput(client, player_input, state);


        // Move the camera to keep the local player centered on screen
        camera_follow(&cam, state->players[local_id].x, state->players[local_id].y, PLAYER_SIZE, PLAYER_SIZE);

        // Draw the map with the camera offset
        render_map(renderer, assets.map_texture, &cam);

        // Draw all active players
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (state->players[i].active)
            {
                Player p = {0};
                p.Hitbox.x = state->players[i].x;
                p.Hitbox.y = state->players[i].y;
                p.Hitbox.w = PLAYER_SIZE;
                p.Hitbox.h = PLAYER_SIZE;
                p.current_frame = state->players[i].current_frame;
                p.direction = state->players[i].direction;

                renderPlayer(renderer, &p, assets.skins[i], &cam);
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
