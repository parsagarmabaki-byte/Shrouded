#include "game.h"
#include "client_network.h"
#include "wall_data.h"
#include "emergency_meeting.h"

void runGame(Client *client, waitForPlayers *lobby, gameState *state)
{
    TTF_Init();
    SDL_Renderer *renderer = lobby->renderer;
    SDL_RenderSetLogicalSize(renderer, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    int local_id = state->local_player_id;
    bool emergency_window_open = false;
    bool is_local_impostor = state->players[local_id].isImpostor != 0;
    bool running = true;
    bool task_map_open = false;
    float accumulator = 0.0f;
    Uint64 last_tick = SDL_GetPerformanceCounter();
    srand(time(NULL));
    bool was_task_active = false;
    bool ui_open;
    int targeted_banner_id = -1;
    float dt;

    Player *player = player_create(state, local_id);
    Task *task = create_task(renderer);
    KillAnimation bodies[MAX_PLAYERS] = {0};
    Camera cam = {0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT};
    SDL_Event event;
    clientInput user_input;
    GameAssets assets = load_assets(renderer);

    SDL_RaiseWindow(lobby->window);
    SDL_SetWindowInputFocus(lobby->window);

    while (running)
    {
        dt = calculate_delta_time(&last_tick);
        process_events(client, renderer, state, task, &event, player, bodies, local_id, &running, &emergency_window_open, is_local_impostor, &task_map_open, &targeted_banner_id);
        collect_packets(client, state, bodies);
        ui_open = emergency_window_open || task_map_open;
        if (state->phase == GAME_RUNNING)
            update_game(client, state, player, task, bodies, &user_input, local_id, ui_open, &was_task_active, dt, &accumulator);

        render_game_phase(client, renderer, state, player, task, bodies, &cam, assets, user_input, local_id, is_local_impostor, task_map_open, &emergency_window_open, dt, targeted_banner_id);
    }

    player_destroy(player);
    destroy_task(task);
    destroy_assets(&assets);
    TTF_Quit();
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

void render_all_players(gameState *state, Player *player, GameAssets assets, Camera *cam, SDL_Renderer *renderer, int local_id)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!state->players[i].active)
            continue;

        Player p = *player;
        int alpha = 255;

        if (i == local_id)
        {
            if (!state->players[i].isAlive)
                alpha = 155;

            p.Hitbox.x = player->Hitbox.x;
            p.Hitbox.y = player->Hitbox.y;
            p.current_frame = player->current_frame;
            p.direction = player->direction;
        }
        else
        {
            if (state->players[local_id].isAlive)
            {
                if (!state->players[i].isAlive)
                    continue;
            }
            else
            {
                if (!state->players[i].isAlive)
                    alpha = 155;
                else
                    alpha = 255;
            }

            p.Hitbox.x = state->players[i].x;
            p.Hitbox.y = state->players[i].y;
            p.current_frame = state->players[i].current_frame;
            p.direction = state->players[i].direction;
        }

        SDL_SetTextureAlphaMod(assets.skins[i], alpha);
        renderPlayer(renderer, &p, assets.skins[i], cam);
        SDL_SetTextureAlphaMod(assets.skins[i], 255);
    }
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

void task_events(SDL_Renderer *renderer, SDL_Event *event, Task *task)
{
    // printf("\nIN TASKS EVENT\n");
    if (!task)
        return;

    if (event->type == SDL_KEYDOWN)
    {
        SDL_Scancode sc = event->key.keysym.scancode;

        // start tasks
        if (sc == SDL_SCANCODE_1)
            start_timer_task(task, renderer, 10.0f);

        if (sc == SDL_SCANCODE_2)
            start_click_task(task, renderer, 25);

        if (sc == SDL_SCANCODE_3)
            start_letter_task(task, renderer);

        if (sc == SDL_SCANCODE_4)
            start_reflex_task(task, renderer);

        if (sc == SDL_SCANCODE_5)
            start_logical_order_task(task, renderer);

        if (sc == SDL_SCANCODE_6)
            start_memory_task(task, renderer);

        // cancel task
        if (sc == SDL_SCANCODE_Q)
        {
            if (task_active_check(task))
                end_task(task, TASK_STATUS_CANCELLED);
        }

        // handle task input
        if (event->key.repeat == 0)
        {
            task_handle_key(task, event->key.keysym.sym);
        }
    }

    // handle click input
    if (event->type == SDL_MOUSEBUTTONDOWN)
    {
        task_handle_click(task, event->button.x, event->button.y, renderer);
    }
}

void update_game(Client *client, gameState *state, Player *player, Task *task, KillAnimation bodies[MAX_PLAYERS], clientInput *user_input, int local_id, bool ui_open, bool *was_task_active, float dt, float *accumulator)
{
    *accumulator += dt;
    TaskType task_type_before_events = task_active_check(task) ? task_get_current_type(task) : TASK_NONE;
    update_player_movement(player, user_input, task_active_check(task), ui_open, accumulator);
    send_player_input(client, state, player, task_active_check(task), ui_open);
    compare_server_position(*state, player, local_id);
    update_kill_animation(bodies, dt);
    update_task_check_completion(client, task, state, local_id, dt, task_type_before_events, was_task_active);
}

float calculate_delta_time(Uint64 *last_tick)
{
    Uint64 current_tick = SDL_GetPerformanceCounter();

    float dt = (float)(current_tick - *last_tick) /
               (float)SDL_GetPerformanceFrequency();

    *last_tick = current_tick;

    return dt;
}

void report_body_events(SDL_Renderer *renderer, Client *client, gameState *state, SDL_Event *event, KillAnimation bodies[MAX_PLAYERS], Player *player)
{
    int target_id = target_report_body(bodies, *player);
    if (event->type == SDL_KEYDOWN)
    {
        if (event->key.keysym.scancode == SDL_SCANCODE_R && target_id != -1 && state->players[state->local_player_id].isAlive)
        {
            request_report_body(client, state, bodies[target_id], target_id);
        }
    }
    else if (event->type == SDL_MOUSEBUTTONDOWN)
    {
        SDL_Rect report_button = {955, 455, 120, 120};
        if (is_hovering(renderer, report_button) && target_id != -1 && state->players[state->local_player_id].isAlive)
        {
            request_report_body(client, state, bodies[target_id], target_id);
        }
    }
}

void emergency_meeting_events(Client *client, gameState *state, SDL_Renderer *renderer, SDL_Event *event, Player *player, bool *emergency_window_open, int local_id)
{
    SDL_Rect emergency_button = {(LOGICAL_SCREEN_WIDTH / 2) - 20, ((LOGICAL_SCREEN_HEIGHT) / 2) - 65, 33, 33};
    if (event->type == SDL_KEYDOWN)
    {
        if (event->key.keysym.scancode == SDL_SCANCODE_E)
        {
            if (collides_with_wall(player->Hitbox.x, player->Hitbox.y) == 2 && state->players[local_id].isAlive)
                *emergency_window_open = true;
        }
    }
    if (*emergency_window_open && event->type == SDL_MOUSEBUTTONDOWN)
    {
        if (is_hovering(renderer, emergency_button))
        {
            printf("\n[CLIENT] Player %d tried to open emergency meeting screen. isAlive=%d\n",
                   state->local_player_id,
                   state->players[state->local_player_id].isAlive);
            request_emergency_meeting(client, state, local_id);
        }
    }
}

void kill_events(Client *client, SDL_Renderer *renderer, gameState *state, SDL_Event *event, bool kill_cooldown, bool is_local_impostor)
{
    SDL_Rect kill_button = {1077, 550, 150, 145};
    if (event->type == SDL_KEYDOWN)
    {
        if (!kill_cooldown && is_local_impostor)
        {
            if (event->key.keysym.scancode == SDL_SCANCODE_K)
            {
                request_kill(client, state);
            }
        }
    }
    if (!kill_cooldown && is_local_impostor && event->type == SDL_MOUSEBUTTONDOWN)
    {
        if (is_hovering(renderer, kill_button))
        {
            request_kill(client, state);
        }
    }
}

void debug_walls(SDL_Renderer *renderer, Camera cam)
{
#ifdef DEBUG_WALLS
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 50);

    for (int row = 0; row <= WALL_MAP_ROWS; row++)
    {
        int y = row * WALL_TILE_SIZE - (int)cam.y;
        SDL_RenderDrawLine(renderer,
                           0 - (int)cam.x, y,
                           (WALL_MAP_COLS * WALL_TILE_SIZE) - (int)cam.x, y);
    }

    for (int col = 0; col <= WALL_MAP_COLS; col++)
    {
        int x = col * WALL_TILE_SIZE - (int)cam.x;
        SDL_RenderDrawLine(renderer,
                           x, 0 - (int)cam.y,
                           x, (WALL_MAP_ROWS * WALL_TILE_SIZE) - (int)cam.y);
    }

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100);

    for (int row = 0; row < WALL_MAP_ROWS; row++)
    {
        for (int col = 0; col < WALL_MAP_COLS; col++)
        {
            if (wall_map[row][col])
            {
                SDL_Rect r = {
                    col * WALL_TILE_SIZE - (int)cam.x,
                    row * WALL_TILE_SIZE - (int)cam.y,
                    WALL_TILE_SIZE,
                    WALL_TILE_SIZE};
                SDL_RenderFillRect(renderer, &r);

                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &r);
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100);
            }
        }
    }
#endif
}

void render_task_map(SDL_Renderer *renderer, Task *task, GameAssets assets, Player *player)
{
    SDL_Rect backdrop = {0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT};
    SDL_Rect map_rect = {252, 56, 776, 620};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170);
    SDL_RenderFillRect(renderer, &backdrop);

    SDL_SetRenderDrawColor(renderer, 10, 15, 22, 235);
    SDL_RenderFillRect(renderer, &map_rect);

    if (assets.map_texture)
    {
        SDL_RenderCopy(renderer, assets.map_texture, NULL, &map_rect);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &map_rect);

    // Exempelmarkörer för tasks
    SDL_Rect marker1 = {320, 140, 24, 24};
    SDL_Rect marker2 = {453, 310, 24, 24};
    SDL_Rect marker3 = {835, 310, 24, 24};
    SDL_Rect marker4 = {675, 140, 24, 24};
    SDL_Rect marker5 = {790, 130, 24, 24};
    SDL_Rect marker6 = {900, 130, 24, 24};
    SDL_Rect marker7 = {825, 570, 24, 24};
    SDL_Rect marker8 = {550, 510, 24, 24};
    SDL_Rect marker9 = {415, 505, 24, 24};

    SDL_SetRenderDrawColor(renderer, 80, 200, 120, 255);
    SDL_RenderFillRect(renderer, &marker1);
    SDL_RenderFillRect(renderer, &marker2);
    SDL_RenderFillRect(renderer, &marker3);
    SDL_RenderFillRect(renderer, &marker4);
    SDL_RenderFillRect(renderer, &marker5);
    SDL_RenderFillRect(renderer, &marker6);
    SDL_RenderFillRect(renderer, &marker7);
    SDL_RenderFillRect(renderer, &marker8);
    SDL_RenderFillRect(renderer, &marker9);

    float scale_x = (float)map_rect.w / GAME_MAP_WIDTH;
    float scale_y = (float)map_rect.h / GAME_MAP_HEIGHT;

    float player_center_x = player->Hitbox.x + player->Hitbox.w / 2.0f;
    float player_center_y = player->Hitbox.y + player->Hitbox.h / 2.0f;

    int marker_x = map_rect.x + (int)(player_center_x * scale_x);
    int marker_y = map_rect.y + (int)(player_center_y * scale_y);

    SDL_Rect player_marker = {
        marker_x - 6,
        marker_y - 6,
        12,
        12};

    SDL_SetRenderDrawColor(renderer, 60, 160, 255, 255);
    SDL_RenderFillRect(renderer, &player_marker);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &player_marker);
}

void process_events(Client *client, SDL_Renderer *renderer, gameState *state, Task *task, SDL_Event *event, Player *player, KillAnimation bodies[MAX_PLAYERS], int local_id, bool *running, bool *emergency_window_open, bool is_local_impostor, bool *task_map_open, int *targeted_banner_id)
{
    while (SDL_PollEvent(event))
    {
        if (state->phase == GAME_RUNNING)
            game_running_events(client, renderer, state, task, event, player, bodies, local_id, running, emergency_window_open, is_local_impostor, task_map_open);
        else if (state->phase == GAME_MEETING)
            game_meeting_events(renderer, *state, event, state->players[local_id].isAlive, targeted_banner_id);
        leave_game_event(client, event, running, emergency_window_open);
    }
}

void leave_game_event(Client *client, SDL_Event *event, bool *running, bool *emergency_window_open)
{
    if (event->type == SDL_KEYDOWN)
    {
        if (event->key.keysym.scancode == SDL_SCANCODE_ESCAPE)
        {
            if (*emergency_window_open)
            {
                *emergency_window_open = false;
            }
            else
            {
                send_leave_message(client);
                *running = false;
            }
        }
    }
}

void game_running_events(Client *client, SDL_Renderer *renderer, gameState *state, Task *task, SDL_Event *event, Player *player, KillAnimation bodies[MAX_PLAYERS], int local_id, bool *running, bool *emergency_window_open, bool is_local_impostor, bool *task_map_open)
{
    if (event->type == SDL_QUIT)
        *running = false;
    if (event->type == SDL_KEYDOWN)
    {
        if (event->key.keysym.scancode == SDL_SCANCODE_M && !task_active_check(task))
        {
            *task_map_open = !*task_map_open;
        }
    }
    task_events(renderer, event, task);
    kill_events(client, renderer, state, event, player->kill_cooldown_active, is_local_impostor);
    emergency_meeting_events(client, state, renderer, event, player, emergency_window_open, local_id);
    report_body_events(renderer, client, state, event, bodies, player);
}

void game_meeting_events(SDL_Renderer *renderer, gameState state, SDL_Event *event, int player_alive, int *targeted_banner_id)
{
    *targeted_banner_id = target_player_banner(renderer, state, event, player_alive, *targeted_banner_id);
    if (*targeted_banner_id != -1)
    {
        printf("\nBANNER IS CLICKED\n");
    }
    handle_send_vote_button(renderer, event, player_alive);
}

void render_game_phase(Client *client, SDL_Renderer *renderer, gameState *state, Player *player, Task *task, KillAnimation bodies[MAX_PLAYERS], Camera *cam, GameAssets assets, clientInput user_input, int local_id, bool is_local_impostor, bool task_map_open, bool *emergency_window_open, float dt, int targeted_banner_id)
{
    if (state->phase == GAME_RUNNING)
    {
        render_game(renderer, state, cam, assets, user_input, player, bodies, task, local_id, dt, is_local_impostor, *emergency_window_open, task_map_open);
    }
    else if (state->phase == GAME_SHOW_ROLE)
    {
        SDL_Texture *role_img = NULL;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, assets.role_art_img, NULL, NULL);
        if (state->players[local_id].isImpostor)
        {
            role_img = assets.killer_img;
        }
        else
        {
            role_img = assets.innocent_img;
        }

        SDL_Rect role_rect;
        role_rect.w = 400;
        role_rect.h = 200;
        role_rect.x = (LOGICAL_SCREEN_WIDTH - role_rect.w) / 2;
        role_rect.y = (LOGICAL_SCREEN_HEIGHT - role_rect.h) / 4;

        SDL_RenderCopy(renderer, role_img, NULL, &role_rect);
    }
    else if (state->phase == GAME_INFO_MEETING)
    {

        SDL_Rect meeting_info_rect;
        meeting_info_rect.w = 1041;
        meeting_info_rect.h = 641;
        meeting_info_rect.x = (LOGICAL_SCREEN_WIDTH / 2) - meeting_info_rect.w / 2;
        meeting_info_rect.y = (LOGICAL_SCREEN_HEIGHT / 2) - meeting_info_rect.h / 2;
        SDL_Texture *meeting_info_texture = NULL;

        if (state->type == MSG_EMERGENCY_MEETING)
        {
            meeting_info_texture = assets.emergency_meeting_info;
        }
        else if (state->type == MSG_BODY_FOUND)
        {
            meeting_info_texture = assets.dead_body_reported_info;
            for (int i = 0; i < MAX_PLAYERS; i++)
            {
                bodies[i].active = false;
            }
        }
        SDL_RenderCopy(renderer, meeting_info_texture, NULL, &meeting_info_rect);
    }
    else if (state->phase == GAME_MEETING)
    {
        int player_reported_id = state->emergency_meeting_reported_id;
        render_emergency_meeting(renderer, assets, state, player_reported_id, targeted_banner_id);
        *emergency_window_open = false;
    }
    else if (state->phase == GAME_CREWMATES_WIN)
    {
        // Rendera crewmate win screen
    }
    else if (state->phase == GAME_IMPOSTOR_WIN)
    {
        // Rendera impostor win screen
    }
    // SDL_Rect submit_button = {260,555,265,75};
    // SDL_Rect skip_button = {760,555,265,75};
    // SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    // SDL_RenderFillRect(renderer,&submit_button);
    // SDL_RenderFillRect(renderer,&skip_button);
    SDL_RenderPresent(renderer);
}

void update_player_direction(Player *player, clientInput *user_input)
{
    if (user_input->up)
        player->direction = DIR_UP;
    if (user_input->down)
        player->direction = DIR_DOWN;
    if (user_input->left)
        player->direction = DIR_LEFT;
    if (user_input->right)
        player->direction = DIR_RIGHT;
}

void update_player_movement(Player *player, clientInput *user_input, bool task_is_active, bool emergency_window_open, float *accumulator)
{
    *user_input = read_input(task_is_active);
    while (*accumulator >= SERVER_TICK_INTERVAL && !emergency_window_open)
    {
        update_player_direction(player, user_input);
        apply_movement(&player->Hitbox.x, &player->Hitbox.y, *user_input, SERVER_TICK_INTERVAL);
        *accumulator -= SERVER_TICK_INTERVAL;
    }
}

static void render_game(SDL_Renderer *renderer, gameState *state, Camera *cam, GameAssets assets, clientInput user_input, Player *player, KillAnimation bodies[MAX_PLAYERS], Task *task, int local_id, float dt, bool is_local_impostor, bool emergency_window_open, bool task_map_open)
{
    run_animations(&player->animation_timer, &player->current_frame, user_input, dt);
    camera_follow(cam, player->Hitbox.x, player->Hitbox.y, PLAYER_SIZE, PLAYER_SIZE);
    render_map(renderer, assets.map_texture, cam);
    debug_walls(renderer, *cam);
    render_all_players(state, player, assets, cam, renderer, local_id);
    render_kill_animation(renderer, bodies, assets, cam);
    if (assets.vignette_img && !is_local_impostor && state->players[local_id].isAlive)
        SDL_RenderCopy(renderer, assets.vignette_img, NULL, NULL);
    if (state->players[local_id].isAlive)
        render_player_ability(renderer, *player, assets, bodies);

    if (is_local_impostor)
    {
        player->kill_cooldown_active = state->players[local_id].kill_cooldown_active;
        render_imposter_ability(renderer, *state, assets.kill_button_active, assets.kill_button_deactive, player->kill_cooldown_active, local_id);
    }

    render_task(renderer, task, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    if (emergency_window_open)
    {
        emergency_meeting_view(renderer, assets.emergency_button_view);
    }
    if (task_map_open)
    {
        render_task_map(renderer, task, assets, player);
    }
}

void send_player_input(Client *client, gameState *state, Player *player, bool task_is_active, bool emergency_window_open)
{
    if (!task_is_active && !emergency_window_open)
    {
        send_input(client, state, player);
    }
}

void update_task_check_completion(Client *client, Task *task, gameState *state, int local_id, float dt, TaskType task_type_before_events, bool *was_task_active)
{
    update_task(task, dt);

    bool task_active_now = task_active_check(task);

    // Detect transition from active to inactive
    if (*was_task_active && !task_active_now)
    {
        // First check: was it cancelled?
        if (task_get_status(task) == TASK_STATUS_CANCELLED)
        {
            printf("[Player %d] Cancelled task\n", local_id);
        }
        // Second check: was it completed?
        else if (task_get_status(task) == TASK_STATUS_COMPLETED)
        {
            int expected_index = state->players[local_id].tasks_completed;
            if (expected_index < TASK_COUNT)
            {
                TaskType expected = state->players[local_id].task_order[expected_index];

                // Correct task completed
                if (task_type_before_events == expected && task_type_before_events != TASK_NONE)
                {
                    send_task_complete(client, local_id, task_type_before_events);
                    printf("[Player %d] Completed correct task (TaskType %d)\n", local_id, task_type_before_events);
                }
                // Wrong task completed
                else
                {
                    printf("[Player %d] Completed wrong task (did %d, needed %d), not counted\n", local_id, task_type_before_events, expected);
                }
            }
        }
    }

    *was_task_active = task_active_now;
}
