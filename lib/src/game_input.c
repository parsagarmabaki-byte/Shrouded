#include "game.h"
#include "game_input.h"
#include "client_network.h"
#include "wall_data.h"
#include "emergency_meeting.h"

static void leave_game_event(GameContext *ctx);
static void win_screen_events(GameContext *ctx);
static void game_running_events(GameContext *ctx);
static void game_meeting_events(GameContext *ctx);
static void report_body_events(GameContext *ctx);
static void emergency_meeting_events(GameContext *ctx);
static void kill_events(GameContext *ctx);
static void task_events(GameContext *ctx);
static void try_start_current_task(GameContext *ctx);
static SDL_Rect win_play_again_rect(void);
static SDL_Rect win_main_menu_rect(void);

void process_events(GameContext *ctx)
{
    while (SDL_PollEvent(&ctx->event))
    {
        if (ctx->event.type == SDL_KEYUP)
            task_handle_keyup(ctx->task, ctx->event.key.keysym.sym);

        // Pausmenyn fångar ESC och klick, resten av spelet ska inte reagera
        if (ctx->pause_menu_open)
        {
            leave_game_event(ctx);
            continue;
        }

        if (ctx->state->phase == GAME_CREWMATES_WIN || ctx->state->phase == GAME_KILLER_WIN)
        {
            win_screen_events(ctx);
            continue;
        }

        if (ctx->state->phase == GAME_RUNNING)
            game_running_events(ctx);
        else if (ctx->state->phase == GAME_MEETING)
            game_meeting_events(ctx);
        leave_game_event(ctx);
    }
}

InputMsg read_input(bool tasks_active)
{
    InputMsg input = {0};
    const Uint8 *key = SDL_GetKeyboardState(NULL);
    if (!tasks_active)
    {
        input.up = key[SDL_SCANCODE_W];
        input.down = key[SDL_SCANCODE_S];
        input.left = key[SDL_SCANCODE_A];
        input.right = key[SDL_SCANCODE_D];
    }
    else
    {
        input.up = 0, input.down = 0, input.left = 0, input.right = 0;
    }
    return input;
}

static void report_body_events(GameContext *ctx)
{
    int body_id = target_report_body(ctx->bodies, *ctx->player);
    if (ctx->event.type == SDL_KEYDOWN)
    {
        if (ctx->event.key.keysym.scancode == SDL_SCANCODE_R && body_id != -1 && ctx->state->players[ctx->state->local_player_id].isAlive && !task_active_check(ctx->task))
        {
            request_report_body(ctx->client, body_id);
        }
    }
    else if (ctx->event.type == SDL_MOUSEBUTTONDOWN)
    {
        SDL_Rect report_button = {1075, 400, 120, 120};
        if (is_hovering(ctx->renderer, report_button) && body_id != -1 && ctx->state->players[ctx->state->local_player_id].isAlive && !task_active_check(ctx->task))
        {
            request_report_body(ctx->client, body_id);
        }
    }
}

static void emergency_meeting_events(GameContext *ctx)
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

static void kill_events(GameContext *ctx)
{
    SDL_Rect kill_button = {1077, 550, 150, 145};
    int target_id = -1;
    if (ctx->event.type == SDL_KEYDOWN)
    {
        if (!ctx->state->kill_cooldown_active && ctx->is_local_impostor)
        {
            if (ctx->event.key.keysym.scancode == SDL_SCANCODE_K)
            {
                target_id = handle_kill_request(ctx->state, ctx->local_id);
                if (target_id != -1)
                    request_kill(ctx->client, target_id);
            }
        }
    }
    if (!ctx->state->kill_cooldown_active && ctx->is_local_impostor && ctx->event.type == SDL_MOUSEBUTTONDOWN)
    {
        if (is_hovering(ctx->renderer, kill_button))
        {
            target_id = handle_kill_request(ctx->state, ctx->local_id);
            if (target_id != -1)
                request_kill(ctx->client, target_id);
        }
    }
}

static SDL_Rect win_play_again_rect(void)
{
    return (SDL_Rect){315, 600, 260, 70};
}

static SDL_Rect win_main_menu_rect(void)
{
    return (SDL_Rect){695, 600, 260, 70};
}

static void leave_game_event(GameContext *ctx)
{
    if (ctx->event.type == SDL_QUIT)
    {
        send_leave_message(ctx->client);
        ctx->running = false;
        return;
    }

    if (ctx->event.type == SDL_KEYDOWN && ctx->event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
    {
        if (ctx->emergency_window_open)
            ctx->emergency_window_open = false;
        else
            ctx->pause_menu_open = !ctx->pause_menu_open;

        return;
    }

    if (ctx->pause_menu_open && ctx->event.type == SDL_MOUSEBUTTONDOWN)
    {
        SDL_Rect resume_rect = {(LOGICAL_SCREEN_WIDTH / 2) - 228, 352, 208, 82};
        SDL_Rect exit_rect = {(LOGICAL_SCREEN_WIDTH / 2) + 20, 352, 208, 82};

        if (is_hovering(ctx->renderer, resume_rect))
            ctx->pause_menu_open = false;
        else if (is_hovering(ctx->renderer, exit_rect))
        {
            send_leave_message(ctx->client);
            ctx->running = false;
        }
    }
}

static void game_running_events(GameContext *ctx)
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
#ifdef DEBUG
            send_debug_win(ctx->client, MSG_DEBUG_CREWMATES_WIN);
#endif
        }
        else if (ctx->event.key.keysym.scancode == SDL_SCANCODE_F2)
        {
#ifdef DEBUG
            send_debug_win(ctx->client, MSG_DEBUG_IMPOSTOR_WIN);
#endif
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

static void game_meeting_events(GameContext *ctx)
{
    ctx->targeted_banner_id = target_player_banner(ctx->renderer, *ctx->state, &ctx->event, ctx->state->players[ctx->local_id].isAlive, ctx->targeted_banner_id);
    handle_send_vote_button(ctx->client, ctx->renderer, &ctx->event, ctx->state->players[ctx->local_id].isAlive, ctx->targeted_banner_id, &ctx->player_voted, ctx->local_id);
}

static void task_events(GameContext *ctx)
{
    if (!ctx->task || ctx->is_local_impostor)
        return;

    if (ctx->event.type == SDL_KEYDOWN)
    {
        SDL_Scancode sc = ctx->event.key.keysym.scancode;

        if (sc == SDL_SCANCODE_E)
        {
            try_start_current_task(ctx);
            return;
        }

        if (sc == SDL_SCANCODE_Q && task_active_check(ctx->task))
        {
            end_task(ctx->task, TASK_STATUS_CANCELLED);
            return;
        }

        if (ctx->event.key.repeat == 0)
            task_handle_key(ctx->task, ctx->event.key.keysym.sym);
    }

    if (ctx->event.type == SDL_MOUSEBUTTONDOWN)
        task_handle_click(ctx->task, ctx->event.button.x, ctx->event.button.y, ctx->renderer);
}

static void try_start_current_task(GameContext *ctx)
{
    if (ctx->event.key.keysym.scancode != SDL_SCANCODE_E)
        return;

    if (!ctx->player || !ctx->task || task_active_check(ctx->task))
        return;

    if (ctx->state->players[ctx->local_id].tasks_completed >= TASK_COUNT)
        return;

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

static void win_screen_events(GameContext *ctx)
{
    if (ctx->event.type == SDL_QUIT)
    {
        send_leave_message(ctx->client);
        ctx->running = false;
        return;
    }

    if (ctx->event.type == SDL_KEYDOWN && ctx->event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
    {
        send_leave_message(ctx->client);
        ctx->running = false;
        return;
    }

    if (ctx->event.type == SDL_MOUSEBUTTONDOWN)
    {
        if (is_hovering(ctx->renderer, win_play_again_rect()))
        {
            send_play_again(ctx->client);
        }
        else if (is_hovering(ctx->renderer, win_main_menu_rect()))
        {
            send_leave_message(ctx->client);
            ctx->return_to_menu = true;
            ctx->running = false;
        }
    }
}
