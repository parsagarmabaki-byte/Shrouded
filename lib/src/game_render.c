
#include "game_render.h"
#include "emergency_meeting.h"
#include "wall_data.h"

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

void render_game_phase(GameContext *ctx)
{
    static gamePhase previous_phase = GAME_LOBBY;
    static Uint32 win_fade_start = 0;
    const Uint32 win_fade_duration = 1500;
    gameState *state = ctx->state;

    handle_phase_transition(ctx, &previous_phase, &win_fade_start);

    switch (state->phase)
    {
    case GAME_RUNNING:
        render_game(ctx);
        break;

    case GAME_SHOW_ROLE:
        render_game_show_role(ctx->renderer, ctx->show_role_asset, state, ctx->local_id);
        break;

    case GAME_INFO_MEETING:
        render_game_info_meeting(ctx);
        break;

    case GAME_MEETING:
        render_emergency_meeting(ctx->renderer, ctx->assets, state, state->emergency_meeting_reported_id, ctx->targeted_banner_id, ctx->timer_meeting_text, ctx->local_id);
        ctx->emergency_window_open = false;
        break;

    case SHOW_VOTE_RESULT:
        render_voting_screen(ctx->renderer, state, ctx->assets, state->voting_result);
        break;

    case GAME_CREWMATES_WIN:
        render_crewmate_win_screen(ctx->renderer, ctx->assets, *state);
        break;

    case GAME_IMPOSTOR_WIN:
        render_killer_win(ctx->renderer, ctx->assets, *state);
        break;

    default:
        break;
    }
    
    render_win_fade(ctx, win_fade_start, win_fade_duration);
    render_pause_menu(ctx->renderer, ctx->assets, ctx->pause_menu_open);

    SDL_RenderPresent(ctx->renderer);
}

static void render_game(GameContext *ctx)
{
    render_world(ctx);
    render_player_overlays(ctx);
    render_game_ui(ctx);
}

void handle_phase_transition(GameContext *ctx, gamePhase *previous_phase, Uint32 *win_fade_start)
{
    gameState *state = ctx->state;

    if (state->phase == *previous_phase)
        return;

    if (state->phase == GAME_CREWMATES_WIN || state->phase == GAME_IMPOSTOR_WIN)
    {
        *win_fade_start = SDL_GetTicks();
    }
    else if (state->phase == GAME_SHOW_ROLE)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            ctx->bodies[i].active = false;
        }
    }

    *previous_phase = state->phase;
}

static void render_win_fade(GameContext *ctx, Uint32 win_fade_start, Uint32 win_fade_duration)
{
    gameState *state = ctx->state;

    if (state->phase != GAME_CREWMATES_WIN && state->phase != GAME_IMPOSTOR_WIN)
        return;

    Uint32 elapsed = SDL_GetTicks() - win_fade_start;

    if (elapsed >= win_fade_duration)
        return;

    Uint8 alpha = (Uint8)(255 - (elapsed * 255 / win_fade_duration));
    SDL_Rect full_screen = {0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT};

    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, alpha);
    SDL_RenderFillRect(ctx->renderer, &full_screen);
}

void render_pause_menu(SDL_Renderer *renderer, GameAssets assets, bool pause_menu_open)
{
    if (!pause_menu_open)
        return;

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
    SDL_Rect exit_rect = {(LOGICAL_SCREEN_WIDTH / 2) + 20, 352, 208, 82};

    if (assets.pause_resume && is_hovering(renderer, resume_rect))
        SDL_RenderCopy(renderer, assets.pause_resume, NULL, &resume_rect);

    if (assets.pause_exit && is_hovering(renderer, exit_rect))
        SDL_RenderCopy(renderer, assets.pause_exit, NULL, &exit_rect);
}

void render_crewmate_win_screen(SDL_Renderer *renderer, GameAssets assets, gameState state)
{
    int impostor_id = -1;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (state.players[i].isImpostor)
        {
            impostor_id = i;
            break;
        }
    }

    if (impostor_id >= 0 && impostor_id < PLAYER_SLOTS && assets.crewmates_win_screens[impostor_id])
    {
        SDL_RenderCopy(renderer, assets.crewmates_win_screens[impostor_id], NULL, NULL);
    }
}

void render_killer_win(SDL_Renderer *renderer, GameAssets assets, gameState state)
{
    int killer_id = -1;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (state.players[i].isImpostor)
        {
            killer_id = i;
            break;
        }
    }

    if (killer_id >= 0 && killer_id < PLAYER_SLOTS && assets.killer_win_screens[killer_id])
    {
        SDL_RenderCopy(renderer, assets.killer_win_screens[killer_id], NULL, NULL);
    }
}

void render_game_show_role(SDL_Renderer *renderer, Game_Show_Role_asset assets, gameState *state, int local_id)
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

void render_world(GameContext *ctx)
{
    gameState *state = ctx->state;
    SDL_Renderer *renderer = ctx->renderer;
    Player *player = ctx->player;
    GameAssets assets = ctx->assets;

    run_animations(&player->animation_timer, &player->current_frame, ctx->user_input, ctx->dt);
    camera_follow(&ctx->cam, player->Hitbox.x, player->Hitbox.y, PLAYER_SIZE, PLAYER_SIZE);

    render_map(renderer, assets.map_texture, &ctx->cam);
    debug_walls(renderer, ctx->cam);
    render_all_players(state, player, ctx->assets, &ctx->cam, renderer, ctx->local_id);
    render_kill_animation(renderer, ctx->bodies, ctx->assets, &ctx->cam);
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

void render_active_task_indicator(SDL_Renderer *renderer, gameState *state, GameAssets assets, Camera *cam, int local_id)
{
    if (state->players[local_id].isImpostor)
        return;

    int current_index =
        state->players[local_id].tasks_completed;

    if (current_index >= TASK_COUNT)
        return;

    TaskType current_task = state->players[local_id].task_order[current_index];

    TaskMarker *marker =
        find_marker(current_task);

    if (!marker)
        return;

    SDL_Rect dst = {marker->world_x - (int)cam->x - 24, marker->world_y - (int)cam->y - 48, 48, 48};

    SDL_RenderCopy(renderer, assets.task_indicator, NULL, &dst);
}

void render_player_overlays(GameContext *ctx)
{
    gameState *state = ctx->state;
    SDL_Renderer *renderer = ctx->renderer;
    Player *player = ctx->player;
    GameAssets *assets = &ctx->assets;

    if (assets->vignette_img && !ctx->is_local_impostor && state->players[ctx->local_id].isAlive)
        SDL_RenderCopy(renderer, assets->vignette_img, NULL, NULL);

    render_active_task_indicator(renderer, state, ctx->assets, &ctx->cam, ctx->local_id);
    render_info_text(renderer, state, ctx->local_id, ctx->small_text);

    if (state->players[ctx->local_id].isAlive && !task_active_check(ctx->task))
        render_player_ability(renderer, *player, ctx->assets, ctx->bodies);

    if (ctx->is_local_impostor)
    {
        player->kill_cooldown_active = state->players[ctx->local_id].kill_cooldown_active;
        render_imposter_ability(renderer, *state, assets->kill_button_active, assets->kill_button_deactive, player->kill_cooldown_active, ctx->local_id);
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

void render_game_ui(GameContext *ctx)
{
    SDL_Renderer *renderer = ctx->renderer;
    gameState *state = ctx->state;
    Player *player = ctx->player;
    GameAssets *assets = &ctx->assets;

    render_task(renderer, ctx->task, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);

    if (ctx->emergency_window_open)
        emergency_meeting_view(renderer, assets->emergency_button_view, assets->emergency_button_hover);

    if (ctx->task_map_open)
        render_task_map(renderer, ctx->task, ctx->assets, player, state);

    render_global_progress_bar(renderer, state);

    if (ctx->task_panel_visible)
        render_task_panel(renderer, state, ctx->local_id, ctx->task, ctx->small_text, ctx->emergency_window_open, ctx->task_map_open, ctx->pause_menu_open);

    if (ctx->controls_visible)
        render_controls_screen(renderer, state, ctx->local_id, ctx->generic_text);
}

void render_game_info_meeting(GameContext *ctx)
{
    gameState *state = ctx->state;
    GameAssets assets = ctx->assets;
    KillAnimation *bodies = ctx->bodies;

    SDL_Rect meeting_info_rect;
    meeting_info_rect.w = 1041;
    meeting_info_rect.h = 641;
    meeting_info_rect.x = (LOGICAL_SCREEN_WIDTH / 2) - meeting_info_rect.w / 2;
    meeting_info_rect.y = (LOGICAL_SCREEN_HEIGHT / 2) - meeting_info_rect.h / 2;
    SDL_Texture *meeting_info_texture = NULL;

    render_world(ctx);
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 120);
    SDL_Rect backdrop = {0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT};
    SDL_RenderFillRect(ctx->renderer, &backdrop);

    if (state->type == MSG_EMERGENCY_MEETING)
    {
        meeting_info_texture = ctx->assets.emergency_meeting_info;
    }
    else if (state->type == MSG_BODY_FOUND)
    {
        meeting_info_texture = assets.dead_body_reported_info;
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            bodies[i].active = false;
        }
    }
    SDL_RenderCopy(ctx->renderer, meeting_info_texture, NULL, &meeting_info_rect);
}

void render_global_progress_bar(SDL_Renderer *renderer, gameState *state)
{
    int active_players = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (state->players[i].active)
        {
            active_players++;
        }
    }

    int total_tasks_required = TASK_COUNT * (active_players - 1);
    if (total_tasks_required <= 0)
    {
        return;
    }

    float progress = (float)state->total_tasks_completed / total_tasks_required;
    if (progress > 1.0f)
    {
        progress = 1.0f;
    }

    int bar_w = 260;
    int bar_h = 20;
    int margin = 20;

    int bar_x = LOGICAL_SCREEN_WIDTH - bar_w - margin;
    int bar_y = margin;

    SDL_Rect bg = {bar_x, bar_y, bar_w, bar_h};
    SDL_Rect fill = {bar_x, bar_y, (int)(bar_w * progress), bar_h};

    if (progress > 0)
    {
        SDL_SetRenderDrawColor(renderer, 80, 200, 120, 255);
        SDL_RenderFillRect(renderer, &fill);
    }

    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
    draw_thick_rect(renderer, bg, 5);
    SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
    draw_thick_rect(renderer, bg, 3);
}

static void render_task_panel(SDL_Renderer *renderer, gameState *state, int local_id, Task *task, Text text, bool emergency_window_open, bool task_map_open, bool pause_menu_open)
{
    if (local_id < 0 || !state->players[local_id].active)
        return;

    if (emergency_window_open || task_map_open || pause_menu_open || task_active_check(task)) // only show if no other ui is open
        return;

    const int panel_x = 10;
    const int panel_y = 10;
    const int panel_w = 260;
    const int item_h = 26;
    const int panel_h = 16 + (TASK_COUNT * item_h);

    SDL_Rect panel = {panel_x, panel_y, panel_w, panel_h};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 130, 130, 130, 100);
    SDL_RenderFillRect(renderer, &panel); // inner fill

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
        bool is_impostor = state->players[state->local_player_id].isImpostor;
        if (!completed && !is_impostor)
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
            rect.h + (i * 2)};

        SDL_RenderDrawRect(renderer, &r);
    }
}

void render_task_map(SDL_Renderer *renderer, Task *task, GameAssets assets, Player *player, gameState *state)
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

    int marker_count = sizeof(markers) / sizeof(markers[0]);

    bool is_impostor = state->players[state->local_player_id].isImpostor;
    if (is_impostor)
    {
        for (int i = 0; i < marker_count; i++)
        {
            SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
            SDL_RenderFillRect(renderer, &markers[i].rect);
        }
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 80, 200, 120, 255);
        TaskType current_task = state->players[state->local_player_id].task_order[state->players[state->local_player_id].tasks_completed];

        for (int i = 0; i < marker_count; i++)
        {
            if (markers[i].type == current_task)
            {
                SDL_RenderFillRect(renderer, &markers[i].rect);
                break;
            }
        }
    }

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
        "Trust nobody."};

    int line_count = sizeof(lines) / sizeof(lines[0]);

    int box_width = (int)(LOGICAL_SCREEN_WIDTH * 0.65f);
    int box_height = (int)(line_count * 30) + 40;
    int box_x = (LOGICAL_SCREEN_WIDTH - box_width) / 2;
    int box_y = (LOGICAL_SCREEN_HEIGHT - box_height) / 2;

    SDL_Rect box = {box_x, box_y, box_width, box_height};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &box);

    SDL_SetRenderDrawColor(renderer, 255, 200, 50, 255);
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
