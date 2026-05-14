#include "game.h"
#include "client_network.h"
#include "wall_data.h"
#include "emergency_meeting.h"
#include "game_render.h"

int runGame(Client *client, waitForPlayers *lobby, gameState *state)
{
    srand(time(NULL));
    GameContext ctx = game_context_init(client, state, lobby);
    ctx.assets = load_assets(ctx.renderer);

    while (ctx.running)
    {
        ctx.dt = calculate_delta_time(&ctx.last_tick);
        process_events(&ctx);
        collect_packets(ctx.client, ctx.state, ctx.bodies);
        ctx.is_local_impostor = ctx.state->players[ctx.local_id].isImpostor != 0;
        ctx.ui_open = ctx.emergency_window_open || ctx.task_map_open || ctx.pause_menu_open;
        if (ctx.state->phase == GAME_RUNNING)
            update_game(&ctx);

        render_game_phase(&ctx);
    }
    if (ctx.small_text)
        text_destroy(ctx.small_text);
    if (ctx.generic_text)
        text_destroy(ctx.generic_text);

    text_destroy(ctx.timer_meeting_text);

    player_destroy(ctx.player);
    destroy_task(ctx.task);
    destroy_assets(&ctx.assets);
    return ctx.return_to_menu;
}

static GameContext game_context_init(Client *client, gameState *state, waitForPlayers *lobby)
{
    GameContext ctx = {0};

    ctx.client = client;
    ctx.state = state;
    ctx.renderer = lobby->renderer;

    SDL_RenderSetLogicalSize(ctx.renderer, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 255);

    ctx.local_id = state->local_player_id;
    ctx.is_local_impostor = state->players[ctx.local_id].isImpostor != 0;
    ctx.running = true;
    ctx.return_to_menu = false;
    ctx.emergency_window_open = false;
    ctx.task_map_open = false;
    ctx.pause_menu_open = false;
    ctx.controls_visible = false;
    ctx.task_panel_visible = true;
    ctx.was_task_active = false;
    ctx.ui_open = false;
    ctx.targeted_banner_id = -1;
    ctx.player_voted = -1;
    ctx.accumulator = 0.0f;
    ctx.last_tick = SDL_GetPerformanceCounter();

    ctx.player = player_create(state, ctx.local_id);
    ctx.task = create_task(ctx.renderer);
    ctx.cam = (Camera){0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT};

    ctx.small_text = text_create(ctx.renderer, "assets/fonts/BebasNeue-Regular.ttf", 18);
    ctx.generic_text = text_create(ctx.renderer, "assets/fonts/BebasNeue-Regular.ttf", 24);
    ctx.timer_meeting_text = text_create(ctx.renderer, "assets/fonts/BebasNeue-Regular.ttf", 40);

    SDL_RaiseWindow(lobby->window);
    SDL_SetWindowInputFocus(lobby->window);

    return ctx;
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
        return "Clean Mess";
    case TASK_ALTERNATE:
        return "Change Lightbulb";
    default:
        return "Unknown";
    }
}

void task_events(GameContext *ctx)
{
    if (!ctx->task)
        return;

    if (ctx->is_local_impostor)
        return;

    if (ctx->event.type == SDL_KEYDOWN)
    {
        SDL_Scancode sc = ctx->event.key.keysym.scancode;

        if (sc == SDL_SCANCODE_E && ctx->player && !task_active_check(ctx->task))
        {
            int tile_type = collides_with_tile(ctx->player->Hitbox.x, ctx->player->Hitbox.y);

            TaskType required_task = ctx->state->players[ctx->local_id].task_order[ctx->state->players[ctx->local_id].tasks_completed];

            switch (required_task)
            {
            case TASK_REFLEX:
                if (tile_type == 7)
                    start_reflex_task(ctx->task, ctx->renderer);
                break;
            case TASK_HOLD:
                if (tile_type == 3)
                    start_hold_task(ctx->task, ctx->renderer, 10);
                break;
            case TASK_MEMORY:
                if (tile_type == 4)
                    start_memory_task(ctx->task, ctx->renderer);
                break;
            case TASK_LOGICAL_ORDER:
                if (tile_type == 5)
                    start_logical_order_task(ctx->task, ctx->renderer);
                break;
            case TASK_CLICK:
                if (tile_type == 6)
                    start_click_task(ctx->task, ctx->renderer, 25);
                break;
            case TASK_TIMER:
                if (tile_type == 8)
                    start_timer_task(ctx->task, ctx->renderer, 15);
                break;
            case TASK_LETTER:
                if (tile_type == 9)
                    start_letter_task(ctx->task, ctx->renderer);
                break;
            case TASK_ALTERNATE:
                if (tile_type == 10)
                    start_alternate_task(ctx->task, ctx->renderer, 25);
                break;
            default:
                break;
            }
        }

        // cancel task
        if (sc == SDL_SCANCODE_Q)
        {
            if (task_active_check(ctx->task))
                end_task(ctx->task, TASK_STATUS_CANCELLED);
        }

        // handle task input
        if (ctx->event.key.repeat == 0)
        {
            task_handle_key(ctx->task, ctx->event.key.keysym.sym);
        }
    }

    // handle click input
    if (ctx->event.type == SDL_MOUSEBUTTONDOWN)
    {
        task_handle_click(ctx->task, ctx->event.button.x, ctx->event.button.y, ctx->renderer);
    }
}

void update_game(GameContext *ctx)
{
    bool task_active = task_active_check(ctx->task);

    ctx->accumulator += ctx->dt;

    ctx->player->kill_cooldown_active =
        ctx->state->players[ctx->local_id].kill_cooldown_active;

    update_player_movement(
        ctx->player,
        &ctx->user_input,
        task_active,
        ctx->ui_open,
        &ctx->accumulator);

    send_player_input(
        ctx->client,
        ctx->state,
        ctx->player,
        task_active,
        ctx->ui_open);

    compare_server_position(*ctx->state, ctx->player, ctx->local_id);
    update_kill_animation(ctx->bodies, ctx->dt);

    update_task_check_completion(
        ctx->client,
        ctx->task,
        ctx->state,
        ctx->local_id,
        ctx->dt,
        &ctx->was_task_active);
}

float calculate_delta_time(Uint64 *last_tick)
{
    Uint64 current_tick = SDL_GetPerformanceCounter();

    float dt = (float)(current_tick - *last_tick) /
               (float)SDL_GetPerformanceFrequency();

    *last_tick = current_tick;

    return dt;
}

void report_body_events(GameContext *ctx)
{
    int target_id = target_report_body(ctx->bodies, *ctx->player);
    if (ctx->event.type == SDL_KEYDOWN)
    {
        if (ctx->event.key.keysym.scancode == SDL_SCANCODE_R && target_id != -1 && ctx->state->players[ctx->state->local_player_id].isAlive && !task_active_check(ctx->task))
        {
            request_report_body(ctx->client, ctx->state, ctx->bodies[target_id], target_id);
        }
    }
    else if (ctx->event.type == SDL_MOUSEBUTTONDOWN)
    {
        SDL_Rect report_button = {1075, 400, 120, 120};
        if (is_hovering(ctx->renderer, report_button) && target_id != -1 && ctx->state->players[ctx->state->local_player_id].isAlive && !task_active_check(ctx->task))
        {
            request_report_body(ctx->client, ctx->state, ctx->bodies[target_id], target_id);
        }
    }
}

void emergency_meeting_events(GameContext *ctx)
{
    SDL_Rect emergency_button = {(LOGICAL_SCREEN_WIDTH / 2) - 20, ((LOGICAL_SCREEN_HEIGHT) / 2) - 65, 33, 33};
    if (ctx->event.type == SDL_KEYDOWN)
    {
        if (ctx->event.key.keysym.scancode == SDL_SCANCODE_E)
        {
            int tile_type = collides_with_tile(ctx->player->Hitbox.x, ctx->player->Hitbox.y);
            if (tile_type == 2 && ctx->state->players[ctx->local_id].isAlive)
                ctx->emergency_window_open = true;
        }
    }
    if (ctx->emergency_window_open && ctx->event.type == SDL_MOUSEBUTTONDOWN)
    {
        if (is_hovering(ctx->renderer, emergency_button))
        {
            printf("\n[CLIENT] Player %d tried to open emergency meeting screen. isAlive=%d\n",
                   ctx->state->local_player_id,
                   ctx->state->players[ctx->state->local_player_id].isAlive);
            request_emergency_meeting(ctx->client, ctx->state, ctx->local_id);
            ctx->player_voted = 0;
        }
    }
}

void kill_events(GameContext *ctx)
{
    SDL_Rect kill_button = {1077, 550, 150, 145};
    if (ctx->event.type == SDL_KEYDOWN)
    {
        if (!ctx->player->kill_cooldown_active && ctx->is_local_impostor)
        {
            if (ctx->event.key.keysym.scancode == SDL_SCANCODE_K)
            {
                request_kill(ctx->client, ctx->state);
            }
        }
    }
    if (!ctx->player->kill_cooldown_active && ctx->is_local_impostor && ctx->event.type == SDL_MOUSEBUTTONDOWN)
    {
        if (is_hovering(ctx->renderer, kill_button))
        {
            request_kill(ctx->client, ctx->state);
        }
    }
}

static TaskMarker *find_marker(TaskType type)
{
    int count = sizeof(markers) / sizeof(markers[0]);

    for (int i = 0; i < count; i++)
    {
        if (markers[i].type == type)
            return &markers[i];
    }

    return NULL;
}

static SDL_Rect win_play_again_rect(void)
{
    return (SDL_Rect){315, 600, 260, 70};
}

static SDL_Rect win_main_menu_rect(void)
{
    return (SDL_Rect){695, 600, 260, 70};
}

static void win_screen_events(Client *client, SDL_Renderer *renderer, SDL_Event *event, bool *running, bool *return_to_menu)
{
    if (event->type == SDL_QUIT)
    {
        send_leave_message(client);
        *running = false;
        return;
    }

    if (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_ESCAPE)
    {
        send_leave_message(client);
        *running = false;
        return;
    }

    if (event->type == SDL_MOUSEBUTTONDOWN)
    {
        if (is_hovering(renderer, win_play_again_rect()))
        {
            send_play_again(client);
        }
        else if (is_hovering(renderer, win_main_menu_rect()))
        {
            send_leave_message(client);
            *return_to_menu = true;
            *running = false;
        }
    }
}

void process_events(GameContext *ctx)
{
    while (SDL_PollEvent(&ctx->event))
    {
        if (ctx->event.type == SDL_KEYUP)
            task_handle_keyup(ctx->task, ctx->event.key.keysym.sym);

        // Pausmenyn fångar ESC och klick, resten av spelet ska inte reagera
        if (ctx->pause_menu_open)
        {
            leave_game_event(ctx->client, ctx->renderer, &ctx->event, &ctx->running, &ctx->emergency_window_open, &ctx->pause_menu_open, ctx->assets);
            continue;
        }

        if (ctx->state->phase == GAME_CREWMATES_WIN || ctx->state->phase == GAME_IMPOSTOR_WIN)
        {
            win_screen_events(ctx->client, ctx->renderer, &ctx->event, &ctx->running, &ctx->return_to_menu);
            continue;
        }

        if (ctx->state->phase == GAME_RUNNING)
            game_running_events(ctx);
        else if (ctx->state->phase == GAME_MEETING)
            game_meeting_events(ctx);
        leave_game_event(ctx->client, ctx->renderer, &ctx->event, &ctx->running, &ctx->emergency_window_open, &ctx->pause_menu_open, ctx->assets);
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
        SDL_Rect exit_rect = {(LOGICAL_SCREEN_WIDTH / 2) + 20, 352, 208, 82};

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

void game_running_events(GameContext *ctx)
{
    if (ctx->event.type == SDL_QUIT)
    {
        send_leave_message(ctx->client);
        ctx->running = false;
        return;
    }

    if (ctx->event.type == SDL_KEYDOWN)
    {
        if (ctx->event.key.keysym.scancode == SDL_SCANCODE_M && !task_active_check(ctx->task))
        {
            ctx->task_map_open = !ctx->task_map_open;
        }
        else if (ctx->event.key.keysym.scancode == SDL_SCANCODE_F1)
        {
            send_debug_win(ctx->client, MSG_DEBUG_CREWMATES_WIN);
        }
        else if (ctx->event.key.keysym.scancode == SDL_SCANCODE_F2)
        {
            send_debug_win(ctx->client, MSG_DEBUG_IMPOSTOR_WIN);
        }
        if (ctx->event.key.keysym.scancode == SDL_SCANCODE_TAB && !task_active_check(ctx->task))
        {
            ctx->task_panel_visible = !ctx->task_panel_visible;
        }
        if (ctx->event.key.keysym.scancode == SDL_SCANCODE_I && !task_active_check(ctx->task))
            ctx->controls_visible = !ctx->controls_visible;
    }

    task_events(ctx);
    kill_events(ctx);
    emergency_meeting_events(ctx);
    report_body_events(ctx);
}

void game_meeting_events(GameContext *ctx)
{
    ctx->targeted_banner_id = target_player_banner(ctx->renderer, *ctx->state, &ctx->event, ctx->state->players[ctx->local_id].isAlive, ctx->targeted_banner_id);
    handle_send_vote_button(ctx->client, ctx->renderer, &ctx->event, ctx->state->players[ctx->local_id].isAlive, ctx->targeted_banner_id, &ctx->player_voted);
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
