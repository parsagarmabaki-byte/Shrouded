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
    bool pause_menu_open = false;
    bool task_panel_visible = true;
    bool controls_visible = false;
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
    Text small_text = text_create(renderer, "assets/fonts/BebasNeue-Regular.ttf", 18);
    Text generic_text = text_create(renderer, "assets/fonts/BebasNeue-Regular.ttf", 24);

    SDL_RaiseWindow(lobby->window);
    SDL_SetWindowInputFocus(lobby->window);

    while (running)
    {
        dt = calculate_delta_time(&last_tick);
        process_events(client, renderer, state, task, &event, player, bodies, local_id, &running, &emergency_window_open, is_local_impostor, &task_map_open, &task_panel_visible, &targeted_banner_id, &pause_menu_open, &controls_visible, assets);
        collect_packets(client, state, bodies);
        ui_open = emergency_window_open || task_map_open || pause_menu_open;
        if (state->phase == GAME_RUNNING)
            update_game(client, state, player, task, bodies, &user_input, local_id, ui_open, &was_task_active, dt, &accumulator);

        render_game_phase(client, renderer, state, player, task, bodies, &cam, assets, user_input, local_id, is_local_impostor, task_map_open, task_panel_visible, &emergency_window_open, dt, targeted_banner_id, pause_menu_open, small_text, controls_visible, generic_text);
    }
    if (small_text)
        text_destroy(small_text);
    if (generic_text)
        text_destroy(generic_text);

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

static const char *task_type_name(TaskType t)
{
    switch (t)
    {
    case TASK_TIMER:
        return "Mannequin Scan";
    case TASK_CLICK:
        return "Clean Crystal";
    case TASK_LETTER:
        return "Write Letter";
    case TASK_REFLEX:
        return "Stoke Fire";
    case TASK_LOGICAL_ORDER:
        return "Organize Shelf";
    case TASK_MEMORY:
        return "Look Into Crystals";
    case TASK_HOLD:
        return "Assemble Tools";
    case TASK_ALTERNATE:
        return "Change Lightbulb";
    default:
        return "Unknown";
    }
}

static void render_global_progress_bar(SDL_Renderer *renderer, gameState *state)
{
    int active_players = 0;
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        if(state->players[i].active)
        {
            active_players++;
        }
    }

    int total_tasks_required = TASK_COUNT * (active_players - 1);
    if(total_tasks_required <= 0)
    {
        return;
    }

    float progress = (float)state->total_tasks_completed / total_tasks_required;
    if(progress > 1.0f)
    {
        progress = 1.0f;
    }

    int bar_w = 260; 
    int bar_h = 20;
    int margin = 20;

    int bar_x = LOGICAL_SCREEN_WIDTH - bar_w - margin;
    int bar_y = margin;

    SDL_Rect bg = { bar_x, bar_y, bar_w, bar_h };
    SDL_Rect fill = { bar_x, bar_y, (int)(bar_w * progress), bar_h };

    if(progress > 0)
    {
        SDL_SetRenderDrawColor(renderer, 80, 200, 120, 255);
        SDL_RenderFillRect(renderer, &fill);
    }

    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
    draw_thick_rect(renderer, bg, 5);
    SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
    draw_thick_rect(renderer, bg, 3);
}

void render_controls_screen(SDL_Renderer *renderer, gameState *state, int local_id, Text text)
{
    if (local_id < 0 || !state->players[local_id].active)
        return;

    const char *lines[] = {
        "Controls:",
        "WASD - Move",
        "E - Interact",
        "Q - Cancel Task",
        "K - Kill (Impostor only)",
        "R - Report Body",
        "I - Toggle Controls/Info Screen",
        "M - Toggle Task Map",
        "TAB - Toggle Task Panel",
        " ",
        "Info:",
        "Complete all tasks or discover and vote out the Impostor to win as Innocent.",
        "Kill all Innocent to win as Impostor.",
        "Trust nobody."
    };

    int line_count = sizeof(lines) / sizeof(lines[0]);

    int box_width = (int)(LOGICAL_SCREEN_WIDTH * 0.65f);
    int box_height = (int)(line_count * 30) + 40;
    int box_x = (LOGICAL_SCREEN_WIDTH - box_width) / 2;
    int box_y = (LOGICAL_SCREEN_HEIGHT - box_height) / 2;

    SDL_Rect box = {box_x, box_y, box_width, box_height};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &box);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    draw_thick_rect(renderer, box, 4);

    SDL_Color white = {255, 255, 255, 255};

    for (int i = 0; i < line_count; i++)
    {
        text_set(text, lines[i], white);
        int text_x = box_x + 20;
        int text_y = box_y + 20 + i * 30;
        text_draw_at(text, text_x, text_y);
    }
}

static void render_task_panel(SDL_Renderer *renderer, gameState *state, int local_id, Task *task, Text text, bool emergency_window_open, bool task_map_open, bool pause_menu_open)
{
    if (local_id < 0 || !state->players[local_id].active)
        return;

    if (emergency_window_open || task_map_open || pause_menu_open || task_active_check(task))   // only show if no other ui is open
        return;

    const int panel_x = 10;
    const int panel_y = 10;
    const int panel_w = 260;
    const int item_h = 26;
    const int panel_h = 16 + (TASK_COUNT * item_h);

    SDL_Rect panel = {panel_x, panel_y, panel_w, panel_h};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 130, 130, 130, 100);
    SDL_RenderFillRect(renderer, &panel);   //inner fill

    SDL_Color white = {255, 255, 255, 255};

    for (int i = 0; i < TASK_COUNT; ++i)
    {
        int y = panel_y + 8 + i * item_h;
        SDL_Rect icon = {panel_x + 8, y, 18, 18};

        // completion status square coloring
        bool completed = (i < state->players[local_id].tasks_completed);
        if (completed)
            SDL_SetRenderDrawColor(renderer, 80, 200, 120, 220);
        else
            SDL_SetRenderDrawColor(renderer, 90, 90, 90, 220);

        SDL_RenderFillRect(renderer, &icon);

        // task label
        char label[64];
        TaskType t = state->players[local_id].task_order[i];
        snprintf(label, sizeof(label), "%d. %s", i + 1, task_type_name(t));

        if (text)
        {
            text_set(text, label, white);
            int text_x = panel_x + 34;
            int text_y = y;
            text_draw_at(text, text_x, text_y);
        }

        // current task outline
        if (!completed)
        {
            SDL_SetRenderDrawColor(renderer, 180, 180, 180, 200);
            SDL_Rect outline = {panel_x + 6, y - 4, panel_w - 16, item_h};
            if (i == state->players[local_id].tasks_completed)
                SDL_RenderDrawRect(renderer, &outline);
        }
    }
}

void draw_thick_rect(SDL_Renderer *renderer, SDL_Rect rect, int thickness)
{
    for (int i = 0; i < thickness; i++)
    {
        SDL_Rect r = {
            rect.x - i,
            rect.y - i,
            rect.w + (i * 2),
            rect.h + (i * 2)
        };

        SDL_RenderDrawRect(renderer, &r);
    }
}

void render_info_text(SDL_Renderer *renderer, gameState *state, int local_id, Text text)
{
    if (local_id < 0 || !state->players[local_id].active)
        return;

    SDL_Color grey = {200, 200, 200, 230};
    text_set(text, "Press \"I\" for info screen", grey);
    text_draw_at(text, 10, LOGICAL_SCREEN_HEIGHT - 30);
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

void task_events(SDL_Renderer *renderer, SDL_Event *event, Task *task, Player *player)
{
    // printf("\nIN TASKS EVENT\n");
    if (!task)
        return;

    if (event->type == SDL_KEYDOWN)
    {
        SDL_Scancode sc = event->key.keysym.scancode;

        if (sc == SDL_SCANCODE_E && player && !task_active_check(task))
        {
            int tile_type = collides_with_wall(player->Hitbox.x, player->Hitbox.y);

            if (tile_type == 7)
                start_reflex_task(task, renderer);
            else if (tile_type == 3)
                start_hold_task(task, renderer, 10);
            else if (tile_type == 4)
                start_memory_task(task, renderer);
            else if (tile_type == 5)
                start_logical_order_task(task, renderer);
            else if (tile_type == 6)
                start_click_task(task, renderer, 25);
            else if (tile_type == 8)
                start_timer_task(task, renderer, 15);
            else if (tile_type == 9)
                start_letter_task(task, renderer);
            else if (tile_type == 10)
                start_alternate_task(task, renderer, 25);
        }

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
    update_player_movement(player, user_input, task_active_check(task), ui_open, accumulator);
    send_player_input(client, state, player, task_active_check(task), ui_open);
    compare_server_position(*state, player, local_id);
    update_kill_animation(bodies, dt);
    update_task_check_completion(client, task, state, local_id, dt, was_task_active);
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
        SDL_Rect report_button = {1075, 400, 120, 120};
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
            int tile_type = collides_with_wall(player->Hitbox.x, player->Hitbox.y);
            if (tile_type == 2 && state->players[local_id].isAlive)
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
    SDL_Rect marker8 = {415, 505, 24, 24};

    SDL_SetRenderDrawColor(renderer, 80, 200, 120, 255);
    SDL_RenderFillRect(renderer, &marker1);
    SDL_RenderFillRect(renderer, &marker2);
    SDL_RenderFillRect(renderer, &marker3);
    SDL_RenderFillRect(renderer, &marker4);
    SDL_RenderFillRect(renderer, &marker5);
    SDL_RenderFillRect(renderer, &marker6);
    SDL_RenderFillRect(renderer, &marker7);
    SDL_RenderFillRect(renderer, &marker8);
    SDL_RenderFillRect(renderer, &marker8);

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

void process_events(Client *client, SDL_Renderer *renderer, gameState *state, Task *task, SDL_Event *event, Player *player, KillAnimation bodies[MAX_PLAYERS], int local_id, bool *running, bool *emergency_window_open, bool is_local_impostor, bool *task_map_open, bool *task_panel_visible, int *targeted_banner_id, bool *pause_menu_open, bool *controls_visible, GameAssets assets)
{
    while (SDL_PollEvent(event))
    {
        if (event->type == SDL_KEYUP)
            task_handle_keyup(task, event->key.keysym.sym);

        // Pausmenyn fångar ESC och klick, resten av spelet ska inte reagera
        if (*pause_menu_open)
        {
            leave_game_event(client, renderer, event, running, emergency_window_open, pause_menu_open, assets);
            continue;
        }

        if (state->phase == GAME_RUNNING)
            game_running_events(client, renderer, state, task, event, player, bodies, local_id, running, emergency_window_open, is_local_impostor, task_map_open, task_panel_visible, controls_visible);
        else if (state->phase == GAME_MEETING)
            game_meeting_events(client, renderer, *state, event, state->players[local_id].isAlive, targeted_banner_id);
        leave_game_event(client, renderer, event, running, emergency_window_open, pause_menu_open, assets);
    }
}

void leave_game_event(Client *client, SDL_Renderer *renderer, SDL_Event *event, bool *running, bool *emergency_window_open, bool *pause_menu_open, GameAssets assets)
{
    if (event->type == SDL_QUIT)
    {
        send_leave_message(client);
        *running = false;
        return;
    }

    if (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_ESCAPE)
    {
        if (*emergency_window_open)
        {
            *emergency_window_open = false;
        }
        else
        {
            // Toggla pausmenyn
            *pause_menu_open = !(*pause_menu_open);
        }
        return;
    }

    // Hantera klick på Resume / Exit-knapparna
    if (*pause_menu_open && event->type == SDL_MOUSEBUTTONDOWN)
    {
        SDL_Rect resume_rect = {(LOGICAL_SCREEN_WIDTH / 2) - 228, 352, 208, 82};
        SDL_Rect exit_rect   = {(LOGICAL_SCREEN_WIDTH / 2) + 20,  352, 208, 82};

        if (is_hovering(renderer, resume_rect))
        {
            *pause_menu_open = false;
        }
        else if (is_hovering(renderer, exit_rect))
        {
            send_leave_message(client);
            *running = false;
        }
    }
}

void game_running_events(Client *client, SDL_Renderer *renderer, gameState *state, Task *task, SDL_Event *event, Player *player, KillAnimation bodies[MAX_PLAYERS], int local_id, bool *running, bool *emergency_window_open,bool is_local_impostor, bool *task_map_open, bool *task_panel_visible, bool *controls_visible)
{
    if (event->type == SDL_QUIT)
        *running = false;
    if (event->type == SDL_KEYDOWN)
    {
        if (event->key.keysym.scancode == SDL_SCANCODE_M && !task_active_check(task))
        {
            *task_map_open = !*task_map_open;
        }
        if (event->key.keysym.scancode == SDL_SCANCODE_TAB)
        {
            *task_panel_visible = !(*task_panel_visible);
        }
        if (event->key.keysym.scancode == SDL_SCANCODE_I)
            *controls_visible = !(*controls_visible);
    }

    task_events(renderer, event, task, player);
    kill_events(client, renderer, state, event, player->kill_cooldown_active, is_local_impostor);
    emergency_meeting_events(client, state, renderer, event, player, emergency_window_open, local_id);
    report_body_events(renderer, client, state, event, bodies, player);
}

void game_meeting_events(Client *client, SDL_Renderer *renderer, gameState state, SDL_Event *event, int player_alive, int *targeted_banner_id)
{
    *targeted_banner_id = target_player_banner(renderer, state, event, player_alive, *targeted_banner_id);
    if (*targeted_banner_id != -1)
    {
        // printf("\nBANNER IS CLICKED\n");
    }
    handle_send_vote_button(client,renderer, event, player_alive, *targeted_banner_id);
}

void render_game_phase(Client *client, SDL_Renderer *renderer, gameState *state, Player *player, Task *task, KillAnimation bodies[MAX_PLAYERS], Camera *cam, GameAssets assets, clientInput user_input, int local_id, bool is_local_impostor, bool task_map_open, bool task_panel_visible, bool *emergency_window_open, float dt, int targeted_banner_id, bool pause_menu_open, Text panel_text, bool controls_visible, Text generic_text)
{
    if (state->phase == GAME_RUNNING)
    {
        render_game(renderer, state, cam, assets, user_input, player, bodies, task, local_id, dt, is_local_impostor, *emergency_window_open, task_map_open, pause_menu_open, task_panel_visible, panel_text, controls_visible, generic_text);
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
        role_rect.w = 380;
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
    else if (state->phase == SHOW_VOTE_RESULT)
    {
        render_voting_screen(renderer,state,assets,state->voting_result);
    }
    else if (state->phase == GAME_CREWMATES_WIN)
    {
        // Rendera crewmate win screen
    }
    else if (state->phase == GAME_IMPOSTOR_WIN)
    {
        // Rendera impostor win screen
    }
    // Rendera pausmeny ovanpå allt annat
    if (pause_menu_open)
    {
        // Halvtransparent svart backdrop
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
        SDL_Rect full = {0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &full);

        // Bakgrundsbild (har redan Resume/Exit inritade)
        SDL_Rect bg_rect = {265, 112, 750, 456};
        if (assets.pause_bg)
            SDL_RenderCopy(renderer, assets.pause_bg, NULL, &bg_rect);

        // Hover-glow: visa den upplysta knappen bara när musen hovrar
        SDL_Rect resume_rect = {(LOGICAL_SCREEN_WIDTH / 2) - 228, 352, 208, 82};
        SDL_Rect exit_rect   = {(LOGICAL_SCREEN_WIDTH / 2) + 20,  352, 208, 82};

        if (assets.pause_resume && is_hovering(renderer, resume_rect))
            SDL_RenderCopy(renderer, assets.pause_resume, NULL, &resume_rect);

        if (assets.pause_exit && is_hovering(renderer, exit_rect))
            SDL_RenderCopy(renderer, assets.pause_exit, NULL, &exit_rect);
    }

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

static void render_game(SDL_Renderer *renderer, gameState *state, Camera *cam, GameAssets assets, clientInput user_input, Player *player, KillAnimation bodies[MAX_PLAYERS], Task *task, int local_id, float dt, bool is_local_impostor, bool emergency_window_open, bool task_map_open, bool pause_menu_open, bool task_panel_visible, Text small_text, bool controls_visible, Text generic_text)
{
    run_animations(&player->animation_timer, &player->current_frame, user_input, dt);
    camera_follow(cam, player->Hitbox.x, player->Hitbox.y, PLAYER_SIZE, PLAYER_SIZE);
    render_map(renderer, assets.map_texture, cam);
    debug_walls(renderer, *cam);
    render_all_players(state, player, assets, cam, renderer, local_id);
    render_kill_animation(renderer, bodies, assets, cam);
    if (assets.vignette_img && !is_local_impostor && state->players[local_id].isAlive)
        SDL_RenderCopy(renderer, assets.vignette_img, NULL, NULL);
    render_info_text(renderer, state, local_id, small_text);
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
        emergency_meeting_view(renderer, assets.emergency_button_view, assets.emergency_button_hover);
    }
    if (task_map_open)
    {
        render_task_map(renderer, task, assets, player);
    }

    render_global_progress_bar(renderer, state);

    if (task_panel_visible)
    {
        render_task_panel(renderer, state, local_id, task, small_text, emergency_window_open, task_map_open, pause_menu_open);
    }
    if (controls_visible)
    {
        render_controls_screen(renderer, state, local_id, generic_text);
    }
}

void send_player_input(Client *client, gameState *state, Player *player, bool task_is_active, bool emergency_window_open)
{
    if (!task_is_active && !emergency_window_open)
    {
        send_input(client, state, player);
    }
}

void update_task_check_completion(Client *client, Task *task, gameState *state, int local_id, float dt, bool *was_task_active)
{
    // Prefer the current task type if it's still active; if the task was ended by an event handler earlier this frame the `type` field will
    // already be cleared. In that case use `last_completed_type` saved by `end_task()` so we know which task actually finished.
    TaskType task_type_before_update = TASK_NONE;

    if (task_active_check(task))
        task_type_before_update = task_get_current_type(task);
    else
        task_type_before_update = task_get_last_type(task);

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
                if (task_type_before_update == expected && task_type_before_update != TASK_NONE)
                {
                    send_task_complete(client, local_id, task_type_before_update);
                    printf("[Player %d] Completed correct task (TaskType %d)\n", local_id, task_type_before_update);
                }
                // Wrong task completed
                else
                {
                    printf("[Player %d] Completed wrong task (did %d, needed %d), not counted\n", local_id, task_type_before_update, expected);
                }
            }
        }
    }

    *was_task_active = task_active_now;
}
