#define SDL_MAIN_HANDLED
#include <SDL2/SDL_image.h>
#include "player_movement.h"
#include "game_map.h"
#include "network_data.h"
#include <math.h>

void apply_movement(float *x, float *y, clientInput input, float dt)
{
    float dx = (input.right - input.left);
    float dy = (input.down - input.up);

    if (dx || dy)
    {
        float inv_len = 1.0f / sqrtf(dx * dx + dy * dy);
        dx *= inv_len;
        dy *= inv_len;
    }

    *x += dx * PLAYER_SPEED * dt;
    *y += dy * PLAYER_SPEED * dt;
}

Player init_player(gameState state, int local_id)
{
    Player player = {{state.players[local_id].x, state.players[local_id].y, PLAYER_SIZE, PLAYER_SIZE}};
    player.current_frame = state.players[local_id].current_frame;
    player.direction = state.players[local_id].direction;
    player.animation_timer = 0.0f;
    player.kill_cooldown_active = state.players[local_id].kill_cooldown_active;
    player.kill_cooldown_end = state.players[local_id].kill_cooldown_start;
    return player;
}

void update_map(SDL_Renderer *renderer, SDL_Texture *Game_map, Player *player, SDL_Texture *player_sprite, Camera *cam)
{
    render_map(renderer, Game_map, cam);
    renderPlayer(renderer, player, player_sprite, cam);
}

void renderPlayer(SDL_Renderer *renderer, Player *player, SDL_Texture *texture, Camera *cam)
{
    SDL_Rect src;
    SDL_Rect dst;

    // --- Pick frame from sprite sheet ---
    src.x = player->current_frame * FRAME_SIZE;
    src.y = player->direction * FRAME_SIZE;
    src.w = FRAME_SIZE;
    src.h = FRAME_SIZE;

    // --- Where to draw on screen ---
    dst.x = (int)player->Hitbox.x;
    dst.y = (int)player->Hitbox.y;

    // Rita spelaren med camera-offset
    dst.x = (int)(player->Hitbox.x - cam->x);
    dst.y = (int)(player->Hitbox.y - cam->y);
    dst.w = (int)player->Hitbox.w;
    dst.h = (int)player->Hitbox.h;

    SDL_RenderCopy(renderer, texture, &src, &dst);
}
