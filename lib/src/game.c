#include "game.h"
#include "client_network.h"
#include "wall_data.h"
#include "emergency_meeting.h"
#include "game_render.h"

static void game_context_cleanup(GameContext *ctx);

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
    game_context_cleanup(&ctx);
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

static void game_context_cleanup(GameContext *ctx)
{
    if (ctx->small_text)
        text_destroy(ctx->small_text);

    if (ctx->generic_text)
        text_destroy(ctx->generic_text);

    if (ctx->timer_meeting_text)
        text_destroy(ctx->timer_meeting_text);

    if (ctx->player)
        player_destroy(ctx->player);

    if (ctx->task)
        destroy_task(ctx->task);

    destroy_assets(&ctx->assets);
}

void update_game(GameContext *ctx)
{
    bool task_active = task_active_check(ctx->task);
    ctx->accumulator += ctx->dt;
    ctx->player->kill_cooldown_active = ctx->state->players[ctx->local_id].kill_cooldown_active;

    update_player_movement(ctx->player, &ctx->user_input, task_active, ctx->ui_open, &ctx->accumulator);
    send_player_input(ctx->client, ctx->state, ctx->player, task_active, ctx->ui_open);
    compare_server_position(*ctx->state, ctx->player, ctx->local_id);
    update_kill_animation(ctx->bodies, ctx->dt);
    update_task_check_completion(ctx->client, ctx->task, ctx->state, ctx->local_id, ctx->dt, &ctx->was_task_active);
}

float calculate_delta_time(Uint64 *last_tick)
{
    Uint64 current_tick = SDL_GetPerformanceCounter();

    float dt = (float)(current_tick - *last_tick) /
               (float)SDL_GetPerformanceFrequency();

    *last_tick = current_tick;

    return dt;
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
