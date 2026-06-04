#define SDL_MAIN_HANDLED
#include "imposter_ability.h"
#include "game_map.h"
#include "network_data.h"
#include "game.h"
#include "player_movement.h"

void render_imposter_ability(SDL_Renderer *renderer, gameState state, SDL_Texture *kill_button_active, SDL_Texture *kill_button_deactive, bool kill_cooldown, int killer_id)
{
    SDL_Rect kill_button = {1050, 520, 200, 200};
    // SDL_Rect button = {1077, 550, 150, 145};
    // SDL_RenderFillRect(renderer,&button);
    if (handle_kill_request(&state, killer_id) != -1 && !kill_cooldown)

        SDL_RenderCopy(renderer, kill_button_active, NULL, &kill_button);
    else
        SDL_RenderCopy(renderer, kill_button_deactive, NULL, &kill_button);
}

bool is_hovering(SDL_Renderer *renderer, SDL_Rect rect)
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    float lx, ly;
    SDL_RenderWindowToLogical(renderer, mx, my, &lx, &ly);

    return (lx >= rect.x &&
            lx <= rect.x + rect.w &&
            ly >= rect.y &&
            ly <= rect.y + rect.h);
}

void activate_kill_cooldown(gameState *state, int local_id)
{
    state->players[local_id].kill_cooldown_start = SDL_GetTicks64();
    state->players[local_id].kill_cooldown_active = true;
}

bool update_kill_cooldown(gameState state, int local_id)
{
    Uint64 now = SDL_GetTicks64();
    if (now - state.players[local_id].kill_cooldown_start >= COOLDOWN)
        return false;
    return true;
}

int handle_kill_request(gameState *state, int killer_id)
{
    playerState imposter = state->players[killer_id];
    float best_distance = KILL_RADIUS * KILL_RADIUS;
    int target_id = -1;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (killer_id == i)
        {
            continue;
        }

        if (state->players[i].isAlive && state->players[i].active)
        {
            float distance = find_kill_target(imposter, state->players[i]);
            if (distance > 0 && distance < best_distance)
            {
                target_id = i;
                best_distance = distance;
            }
        }
    }
    if (target_id != -1 && !imposter.kill_cooldown_active)
    {        
        return target_id;
    }
    return -1;
}

float find_kill_target(playerState imposter, playerState innocent)
{
    float fy, fx = 0;
    get_forward_vector(imposter.direction, &fx, &fy);
    float dx = innocent.x - imposter.x;
    float dy = innocent.y - imposter.y;
    float dist_sq = dx * dx + dy * dy;
    // printf("\n[TARGET %d] dx=%.2f dy=%.2f dist_sq=%.2f\n", innocent.player_id ,dx, dy, dist_sq);

    if (dist_sq > KILL_RADIUS * KILL_RADIUS)
    {
        // printf(" -> too far\n");
        return -1;
    }
    float len = sqrtf(dist_sq);
    float vx = dx / len;
    float vy = dy / len;

    float dot = fx * vx + fy * vy;
    // printf(" -> dot=%.2f (fx=%.2f fy=%.2f)\n", dot, fx, fy);

    if (dot < 0.2f)
    {
        // printf(" -> not in front\n");
        return -1;
    }
    // printf(" -> VALID target\n");

    return dist_sq;
}

void get_forward_vector(Direction direction, float *fx, float *fy)
{
    *fx = 0.0f;
    *fy = 0.0f;

    if (direction == DIR_UP)
        *fy = -1.0f;
    if (direction == DIR_DOWN)
        *fy = 1.0f;
    if (direction == DIR_LEFT)
        *fx = -1.0f;
    if (direction == DIR_RIGHT)
        *fx = 1.0f;
}

void start_kill_animation(KillAnimation *anim, int killer_id, int victim_id, float x, float y)
{
    anim->active = true;
    anim->killer_id = killer_id;
    anim->victim_id = victim_id;
    anim->x = x;
    anim->y = y;
    anim->current_frame = 0;
    anim->animation_timer = 0.0f;
}

void update_kill_animation(KillAnimation bodies[MAX_PLAYERS], float dt)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!bodies[i].active)
            continue;

        bodies[i].animation_timer += dt;
        if (bodies[i].animation_timer > 0.05f) // snabb animation
        {
            if (bodies[i].current_frame < 24)
            {
                bodies[i].current_frame++;
                bodies[i].animation_timer = 0.0f;
            }
        }
    }
}

int target_report_body(KillAnimation bodies[MAX_PLAYERS], Player player)
{
    int target = -1;
    float shortest_distance = KILL_RADIUS * KILL_RADIUS;

    float player_center_x = player.Hitbox.x + player.Hitbox.w / 2.0f;
    float player_center_y = player.Hitbox.y + player.Hitbox.h / 2.0f;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!bodies[i].active)
            continue;

        float dx = player_center_x - bodies[i].x;
        float dy = player_center_y - bodies[i].y;
        float dist_sq = dx * dx + dy * dy;
        if (dist_sq < shortest_distance)
        {
            target = i;
            shortest_distance = dist_sq;
        }
    }
    return target;
}

bool find_target_report_body(Position bodies, int player_x, int player_y)
{
    float shortest_distance = KILL_RADIUS * KILL_RADIUS;

    float player_center_x = player_x + 20 / 2.0f;
    float player_center_y = player_y + 20 / 2.0f;
    
    float dx = player_center_x - bodies.x;
    float dy = player_center_y - bodies.y;
    float dist_sq = dx * dx + dy * dy;

    if (dist_sq < shortest_distance)
    {
       return true;
    }
    return false;
}

void render_player_ability(SDL_Renderer *renderer, Player player, GameAssets assets, KillAnimation bodies[MAX_PLAYERS])
{
    SDL_Rect picture_size = {1075, 400, 150, 150};
    SDL_Texture *report_body = assets.report_button_deactive;
    if (target_report_body(bodies, player) != -1)
    {
        picture_size.x -= 15;
        picture_size.y -= 15;
        picture_size.w += 30;
        picture_size.h += 30;
        report_body = assets.report_button_active;
    }
    SDL_RenderCopy(renderer, report_body, NULL, &picture_size);
}

void render_kill_animation(SDL_Renderer *renderer, KillAnimation bodies[MAX_PLAYERS], GameAssets assets, Camera *cam)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!bodies[i].active)
            continue;
        ;

        SDL_Rect src = {
            (bodies[i].current_frame % 5) * FRAME_SIZE,
            (bodies[i].current_frame / 5) * FRAME_SIZE,
            FRAME_SIZE,
            FRAME_SIZE};

        SDL_Rect dst = {
            (int)(bodies[i].x - cam->x),
            (int)(bodies[i].y - cam->y),
            PLAYER_SIZE,
            PLAYER_SIZE};

        // printf("\nRENDER BODY client=%d i=%d active=%d x=%.1f y=%.1f cam=(%.1f, %.1f)\n",
        //     //    /* skicka in local_id hit till funktionen om du måste */,
        //        i,i,
        //        bodies[i].active,
        //        bodies[i].x,
        //        bodies[i].y,
        //        cam->x,
        //        cam->y);
        SDL_RenderCopy(renderer, assets.dead_skins[bodies[i].victim_id], &src, &dst);
    }
}
