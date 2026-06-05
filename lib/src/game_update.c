#include "game.h"
#include "game_update.h"
#include "client_network.h"
#include "player_movement.h"

void update_game(GameContext *ctx)
{
    bool task_active = task_active_check(ctx->task);
    ctx->accumulator += ctx->dt;
    ctx->player->kill_cooldown_active = ctx->state->players[ctx->local_id].kill_cooldown_active;

    update_player_movement(ctx->player, &ctx->user_input, task_active, ctx->ui_open, &ctx->accumulator);
    send_player_input(ctx->client, ctx->player, ctx->user_input, task_active, ctx->ui_open);
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

void update_player_direction(Player *player, InputMsg *user_input)
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

void update_player_movement(Player *player, InputMsg *user_input, bool task_is_active, bool emergency_window_open, float *accumulator)
{
    *user_input = read_input(task_is_active);
    while (*accumulator >= SERVER_TICK_INTERVAL && !emergency_window_open)
    {
        update_player_direction(player, user_input);
        apply_movement(&player->Hitbox.x, &player->Hitbox.y, *user_input, SERVER_TICK_INTERVAL);
        *accumulator -= SERVER_TICK_INTERVAL;
    }
}

void send_player_input(Client *client, Player *player, InputMsg input, bool task_is_active, bool emergency_window_open)
{
    if (!task_is_active && !emergency_window_open)
    {
        send_input(client, player, input);
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
