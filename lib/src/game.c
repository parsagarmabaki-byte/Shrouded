#include "game.h"

// Must match PLAYER_SPEED in server.c for prediction to stay in sync
// Reads keyboard state and sends the local player's input to the server every frame.
// The server uses this to update the authoritative player position.
void sendInput(Client *client, gameState *state, Player *player)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    clientInput input = {0};
    input.type = MSG_CLIENT_INPUT;
    input.player_id = state->local_player_id;
    input.up = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    input.down = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    input.left = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    input.right = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    input.current_frame = player->current_frame;
    input.direction = player->direction;

    send_client_input(client->socket, client->serverAddr, &input);
}

void collect_client_data(Client *client, gameState *state, Player *player, int local_id)
{
    int got_state = -1;
    while (receive_game_state(client->socket, client->recievepacket, state) == 0)
    {
        got_state = 0;
    }

    if (got_state == 0)
    {
        float dx = state->players[local_id].x - player->Hitbox.x;
        float dy = state->players[local_id].y - player->Hitbox.y;

        if (fabsf(dx) > 6.0f)
            player->Hitbox.x = state->players[local_id].x;
        if (fabsf(dy) > 6.0f)
            player->Hitbox.y = state->players[local_id].y;
    }
}
clientInput read_input(bool tasks_active)
{
    clientInput input = {0};
    const Uint8 *key = SDL_GetKeyboardState(NULL);
    if (!tasks_active)
    {
        input.up = key[SDL_SCANCODE_W];
        input.down = key[SDL_SCANCODE_S];
        input.left = key[SDL_SCANCODE_A];
        input.right = key[SDL_SCANCODE_D];
        input.kill = key[SDL_SCANCODE_K];
    }
    else
    {
        input.up = 0, input.down = 0, input.left = 0, input.right = 0;
    }
    return input;
}
void run_animations(float *animation_timer, int *current_frame, clientInput input, float dt)
{
    bool moving = input.up || input.down || input.left || input.right;
    if (moving)
    {
        (*animation_timer) += dt;
        if ((*animation_timer) > 0.1f)
        {
            (*current_frame)++;

            if ((*current_frame) >= 10)
                (*current_frame) = 0;

            (*animation_timer) = 0.0f;
        }
    }
    else
    {
        (*current_frame) = 2;
    }
}
void render_all_players(gameState *state, Player player, GameAssets assets, Camera *cam, SDL_Renderer *renderer, int local_id)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (state->players[i].active)
        {
            Player p = player;
            if (i == local_id)
            {
                p.Hitbox.x = player.Hitbox.x;
                p.Hitbox.y = player.Hitbox.y;
                p.current_frame = player.current_frame;
                p.direction = player.direction;
            }
            else
            {
                p.Hitbox.x = state->players[i].x;
                p.Hitbox.y = state->players[i].y;
                p.current_frame = state->players[i].current_frame;
                p.direction = state->players[i].direction;
            }

            renderPlayer(renderer, &p, assets.skins[i], cam);
        }
    }
}
// Main game loop. Runs after the lobby phase when the server signals GAME_RUNNING.
// Handles input, client-side prediction, receiving server state, and rendering.
void runGame(Client *client, waitForPlayers *lobby, gameState *state)
{
    bool local_player_is_impostor = false;
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
    Player player = init_player(*state, local_id);

    // initialize in-game tasks
    Task task;
    init_task(&task);
    int score = 0;

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
        if (state->phase == GAME_SHOW_ROLE)
        {
            SDL_Texture *role_img;
            collect_client_data(client, state, &player, local_id);

            SDL_RenderClear(renderer);

            if(state->players[local_id].isImpostor)
            {
                role_img = assets.killer_img;
            }
            else
            {
                role_img = assets.innocent_img;
            }

            SDL_RenderCopy(renderer, role_img, NULL, NULL);
            SDL_RenderPresent(renderer);
            continue;   // hoppa till nästa loop-iteration
        }

        // Delta time — time since last frame in seconds
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency();
        last = now;

        // Handle window and keyboard events
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    send_leave(client->socket, client->serverAddr);
                    running = false;
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_E)
                {
                    start_timer_task(&task, 10.0f);
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_Q)
                {
                    if (task.active)
                    {
                        cancel_task(&task);
                    }
                }
            }
        }

        local_player_is_impostor = state->players[local_id].isImpostor != 0;

        accumulator += dt;
        clientInput user_input = read_input(task.active);

        while (accumulator >= SERVER_TICK_INTERVAL) // SERVER TICK ÄR 0.016f
        {
            if (user_input.up)
                player.direction = DIR_UP;
            if (user_input.down)
                player.direction = DIR_DOWN;
            if (user_input.left)
                player.direction = DIR_LEFT;
            if (user_input.right)
                player.direction = DIR_RIGHT;

            apply_movement(&player.Hitbox.x, &player.Hitbox.y, user_input, SERVER_TICK_INTERVAL);
            accumulator -= SERVER_TICK_INTERVAL;
        }

        run_animations(&player.animation_timer, &player.current_frame, user_input, dt);
        if (!task.active)
        {
            sendInput(client, state, &player);
        }
        collect_client_data(client, state, &player, local_id);

        update_task(&task, dt);

        // Move the camera to keep the local player centered on screen
        camera_follow(&cam, player.Hitbox.x, player.Hitbox.y, PLAYER_SIZE, PLAYER_SIZE);

        // Draw the map with the camera offset
        render_map(renderer, assets.map_texture, &cam);

        // Draw all active players
        render_all_players(state, player, assets, &cam, renderer, local_id);

        if (assets.vignette_img && !local_player_is_impostor)
            SDL_RenderCopy(renderer, assets.vignette_img, NULL, NULL);

        render_task(renderer, &task);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(assets.map_texture);
    SDL_DestroyTexture(assets.vignette_img);
    SDL_DestroyTexture(assets.innocent_img);  
    SDL_DestroyTexture(assets.killer_img);
    for (int i = 0; i < PLAYER_SLOTS; i++)
        SDL_DestroyTexture(assets.skins[i]);
}
