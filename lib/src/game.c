#include "game.h"
#include "game_update.h"
#include "client_network.h"
#include "wall_data.h"
#include "emergency_meeting.h"
#include "game_render.h"

static void game_context_cleanup(GameContext *ctx);
static GameContext game_context_init(Client *client, gameState *state, waitForPlayers *lobby);

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
