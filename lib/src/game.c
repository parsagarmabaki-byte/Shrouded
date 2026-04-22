#include "game.h"
#include "client_network.h"
#include "wall_data.h"

// Must match PLAYER_SPEED in server.c for prediction to stay in sync
// Reads keyboard state and sends the local player's input to the server every frame.
// The server uses this to update the authoritative player position.

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
    local_player_is_impostor = state->players[local_id].isImpostor != 0;
    bool kill_cooldown = false;
    SDL_Rect kill_button = {1400, 800, 442, 181};
    Player player = init_player(*state, local_id);

    // initialize in-game tasks
    Task task;
    init_task(&task, renderer);
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
            collect_packets(client,state);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer,assets.role_art_img,NULL,NULL);
            if (state->players[local_id].isImpostor)
            {
                role_img = assets.killer_img;
            }
            else
            {
                role_img = assets.innocent_img;
            }

            SDL_Rect role_rect;
            role_rect.w = 400;                                       // bredd i logiska pixlar
            role_rect.h = 200;                                       // höjd i logiska pixlar
            role_rect.x = (LOGICAL_SCREEN_WIDTH  - role_rect.w) / 2; // centrera horisontellt
            role_rect.y = (LOGICAL_SCREEN_HEIGHT - role_rect.h) / 4; //  vertikalt

            SDL_RenderCopy(renderer, role_img, NULL, &role_rect);
            SDL_RenderPresent(renderer);
            continue; // hoppa till nästa loop-iteration
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
                    send_leave_message(client->socket, client->serverAddr);
                    running = false;
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_1)
                {
                    start_timer_task(&task, renderer, 10.0f);
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_2)
                {
                    start_click_task(&task, renderer, 25);
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_3)
                {
                    start_type_task(&task, renderer);
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_Q)
                {
                    if (task.active)
                    {
                        cancel_task(&task);
                    }
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_K && !kill_cooldown && local_player_is_impostor)
                {
                    request_kill(client, state);
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                if (is_hovering(renderer, kill_button) && event.button.button == SDL_BUTTON_LEFT && !kill_cooldown && local_player_is_impostor)
                {
                    request_kill(client, state);
                }
            }

            if (task.active && task.type == TASK_TYPE)
            {
                if (event.type == SDL_KEYDOWN)
                {
                    char expected = task.target_string[task.current_index];

                    SDL_Keycode key = event.key.keysym.sym;
                    char pressed = (char)SDL_toupper(key);

                    if (pressed == expected)
                    {
                        task.current_index++;
                    }
                    else
                    {
                        task.current_index = 0;
                    }
                }
            }

            if (task.active && task.type == TASK_CLICK)
            {
                if (event.type == SDL_MOUSEBUTTONDOWN)
                {
                    task.click_count++;
                }
            }
        }

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
        // KillEventMsg msg = {0};
        // collect_kill_msg(client,&msg);
        run_animations(&player.animation_timer, &player.current_frame, user_input, dt);
        if (!task.active)
        {
            send_input(client, state, &player);
        }
        // collect_client_data(client, state, &player, local_id);
        collect_packets(client, state);

        //update active task
        update_task(&task, dt);

        // Move the camera to keep the local player centered on screen
        camera_follow(&cam, player.Hitbox.x, player.Hitbox.y, PLAYER_SIZE, PLAYER_SIZE);

        // Draw the map with the camera offset
        render_map(renderer, assets.map_texture, &cam);

                #ifdef DEBUG_WALLS
SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);


    // --- 1. Rita Rutnätet (Grid) ---
    // Vi använder en ljus färg med låg opacitet för att det inte ska störa för mycket
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 50);


    for (int row = 0; row <= WALL_MAP_ROWS; row++) {
        // Horisontella linjer
        int y = row * WALL_TILE_SIZE - (int)cam.x; // OBS: Kontrollera om cam.y ska vara här
        // Korrigering: Det ska vara cam.y för rader
        y = row * WALL_TILE_SIZE - (int)cam.y;
        SDL_RenderDrawLine(renderer,
            0 - (int)cam.x, y,
            (WALL_MAP_COLS * WALL_TILE_SIZE) - (int)cam.x, y);
    }


    for (int col = 0; col <= WALL_MAP_COLS; col++) {
        // Vertikala linjer
        int x = col * WALL_TILE_SIZE - (int)cam.x;
        SDL_RenderDrawLine(renderer,
            x, 0 - (int)cam.y,
            x, (WALL_MAP_ROWS * WALL_TILE_SIZE) - (int)cam.y);
    }


    // --- 2. Rita Väggarna (Röda rutor) ---
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100);


    for (int row = 0; row < WALL_MAP_ROWS; row++) {
        for (int col = 0; col < WALL_MAP_COLS; col++) {
            if (wall_map[row][col]) {
                SDL_Rect r = {
                    col * WALL_TILE_SIZE - (int)cam.x,
                    row * WALL_TILE_SIZE - (int)cam.y,
                    WALL_TILE_SIZE,
                    WALL_TILE_SIZE
                };
                SDL_RenderFillRect(renderer, &r);
               
                // Valfritt: Rita en starkare kant runt just vägg-rutan
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &r);
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100); // Återställ alpha
            }
        }
    }


    #endif

        // Draw all active players
        render_all_players(state, player, assets, &cam, renderer, local_id);

        if (local_player_is_impostor)
        {
            // Sync kill cooldown from server state
            player.kill_cooldown_active = state->players[local_id].kill_cooldown_active;
            // printf("\n%d\n",player.kill_cooldown_active);
            render_imposter_ability(renderer, assets.kill_button_img, player.kill_cooldown_active);
        }
        if (assets.vignette_img && !local_player_is_impostor)
            SDL_RenderCopy(renderer, assets.vignette_img, NULL, NULL);

        TTF_Init();
        render_task(renderer, &task);
        SDL_RenderPresent(renderer);
    }

    destroy_task(&task);
    SDL_DestroyTexture(assets.map_texture);
    SDL_DestroyTexture(assets.vignette_img);
    SDL_DestroyTexture(assets.innocent_img);
    SDL_DestroyTexture(assets.killer_img);
    SDL_DestroyTexture(assets.role_art_img);
    for (int i = 0; i < PLAYER_SLOTS; i++)
        SDL_DestroyTexture(assets.skins[i]);
    TTF_Quit();
}
