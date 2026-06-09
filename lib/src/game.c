#include "game.h"
#include "game_update.h"
#include "client_network.h"
#include "wall_data.h"
#include "emergency_meeting.h"
#include "game_render.h"
#include "time.h"

static void game_context_cleanup(GameContext *ctx);
static GameContext game_context_init(Client *client, gameState *state, waitForPlayers *lobby, AudioAssets *audio);

static SDL_Texture *load_role_texture(SDL_Renderer *renderer, int local_id, bool is_impostor)
{
    char path[128];
    if (is_impostor)
        snprintf(path, sizeof(path), "assets/images/show_role_assets/player%d_killer.png", local_id);
    else
        snprintf(path, sizeof(path), "assets/images/show_role_assets/player%d_innocent.png", local_id);
    return loading_img(renderer, path);
}

static void refresh_role_texture_on_transition(GameContext *ctx)
{
    if (ctx->state->phase != GAME_SHOW_ROLE || ctx->prev_phase == GAME_SHOW_ROLE)
        return;
    if (ctx->player_role)
        SDL_DestroyTexture(ctx->player_role);
    ctx->player_role = load_role_texture(ctx->renderer, ctx->local_id, ctx->is_local_impostor);
}

int runGame(Client *client, waitForPlayers *lobby, gameState *state, AudioAssets *audio)
{
    srand(time(NULL));
    GameContext ctx = game_context_init(client, state, lobby, audio);
    render_game_show_role(ctx.renderer, ctx.player_role);
    SDL_RenderPresent(ctx.renderer);
    ctx.assets = load_assets(ctx.renderer);

    while (ctx.running)
    {
        ctx.dt = calculate_delta_time(&ctx.last_tick);
        process_events(&ctx);
        collect_packets(ctx.client, ctx.state, ctx.bodies, ctx.audio, &ctx.targeted_banner_id, &ctx.player_voted);
        ctx.is_local_impostor = ctx.state->players[ctx.local_id].isKiller != 0;
        refresh_role_texture_on_transition(&ctx);
        ctx.prev_phase = ctx.state->phase;
        ctx.ui_open = ctx.emergency_window_open || ctx.task_map_open || ctx.pause_menu_open;
        if (ctx.state->phase == GAME_RUNNING)
            update_game(&ctx);

        render_game_phase(&ctx);
    }
    game_context_cleanup(&ctx);
    return ctx.return_to_menu;
}

static GameContext game_context_init(Client *client, gameState *state, waitForPlayers *lobby, AudioAssets *audio)
{
    GameContext ctx = {0};

    ctx.client = client;
    ctx.state = state;
    ctx.audio = audio;
    ctx.renderer = lobby->renderer;

    SDL_RenderSetLogicalSize(ctx.renderer, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 255);

    ctx.local_id = state->local_player_id;
    ctx.is_local_impostor = state->players[ctx.local_id].isKiller != 0;
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
    ctx.player_role = load_role_texture(ctx.renderer, ctx.local_id, ctx.is_local_impostor);
    ctx.prev_phase = state->phase;

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

    destroy_assets(&ctx->assets, ctx->player_role);
}
