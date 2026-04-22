#define SDL_MAIN_HANDLED
#include "imposter_ability.h"
#include "game_map.h"
#include "network_data.h"
#include "game.h"

void render_imposter_ability(SDL_Renderer *renderer, SDL_Texture *kill_button_img, bool kill_cooldown)
{
    SDL_Rect kill_button = {1000, 580, 230, 100};

    SDL_SetTextureColorMod(kill_button_img, 255, 255, 255);
    SDL_SetTextureAlphaMod(kill_button_img, 255);

    if (!kill_cooldown)
    {
        bool hovering = is_hovering(renderer, kill_button);

        if (hovering)
        {
            SDL_SetTextureColorMod(kill_button_img, 255, 220, 220);
        }
    }
    else
    {
        SDL_SetTextureAlphaMod(kill_button_img, 150);
    }

    SDL_RenderCopy(renderer, kill_button_img, NULL, &kill_button);
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
            float distance = find_kill_target(imposter,state->players[i]);
            if (distance > 0 && distance < best_distance)
            {
                target_id = i;
                best_distance = distance;
            }
        }
    }
    if (target_id != -1 && !imposter.kill_cooldown_active)
    {
        activate_kill_cooldown(state, killer_id);
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

    if (dot < 0.5f)
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

void update_kill_animation(KillAnimation *anim, float dt)
{
    if (!anim->active)
        return;

    anim->animation_timer += dt;
    if (anim->animation_timer > 0.05f) // snabb animation
    {
        if (anim->current_frame < 24)
        {
            anim->current_frame++;
            anim->animation_timer = 0.0f;
        }
    }
}

void render_kill_animation(SDL_Renderer *renderer, KillAnimation *anim, GameAssets assets, Camera *cam)
{
    if (!anim->active)
        return;

    SDL_Rect src = {
        (anim->current_frame % 5) * FRAME_SIZE,
        (anim->current_frame / 5) * FRAME_SIZE,
        FRAME_SIZE,
        FRAME_SIZE};

    SDL_Rect dst = {
        (int)(anim->x - cam->x),
        (int)(anim->y - cam->y),
        PLAYER_SIZE,
        PLAYER_SIZE};

    SDL_RenderCopy(renderer, assets.dead_skins[anim->victim_id], &src, &dst);
}
