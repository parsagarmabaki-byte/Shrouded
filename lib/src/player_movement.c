#define SDL_MAIN_HANDLED
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include "player_movement.h"
#include "game_map.h"
#include "network_data.h"
#include "wall_data.h"
#include <math.h>

// ADT: konstruktör

/* 
    player_create() allokerar minne på heapen med malloc, anropar befintliga init_player() för att fylla structen, 
    och returnerar en pekare, 
    medan player_destroy() frigor minnet med free.
*/
Player *player_create(gameState *state, int local_id)
{
    Player *p = malloc(sizeof(Player));
    if (!p)
    {
        printf("player_create: malloc failed\n");
        return NULL;
    }
    *p = init_player(*state, local_id);
    return p;
}

// ADT: destruktör
void player_destroy(Player *p)
{
    if (p)
        free(p);
}

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

    float move = PLAYER_SPEED * dt;

    // Testa X och Y separat så spelaren kan glida längs väggar

    float new_x = *x + dx * move;

    if (collides_with_wall(new_x, *y) != 1)
    {
        *x = new_x;
    }

    float new_y = *y + dy * move;

    if (collides_with_wall(*x, new_y) != 1)
    {
        *y = new_y;
    }
}

void compare_server_position(gameState state, Player *player,int local_id)
{
    float dx = state.players[local_id].x - player->Hitbox.x;
    float dy = state.players[local_id].y - player->Hitbox.y;

    if (fabsf(dx) > 6.0f)
        player->Hitbox.x = state.players[local_id].x;
    if (fabsf(dy) > 6.0f)
        player->Hitbox.y = state.players[local_id].y;
}

Player init_player(gameState state, int local_id)
{
    Player player = {{state.players[local_id].x, state.players[local_id].y, HITBOX_SIZE, HITBOX_SIZE}};
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
    SDL_Rect dst_render;
    SDL_Rect dst_hitbox;
    Uint8 old_r, old_g, old_b, old_a;

    src.x = player->current_frame * FRAME_SIZE;
    src.y = player->direction * FRAME_SIZE;
    src.w = FRAME_SIZE;
    src.h = FRAME_SIZE;

    // --- 3. Beräkna var BILDEN ska ritas ---
    int offset_x = (PLAYER_SIZE - (int)player->Hitbox.w) / 2;
    int offset_y = (PLAYER_SIZE - (int)player->Hitbox.h) / 2;

    dst_render.x = (int)(player->Hitbox.x - cam->x) - offset_x;
    dst_render.y = (int)(player->Hitbox.y - cam->y) - offset_y;
    dst_render.w = PLAYER_SIZE;
    dst_render.h = PLAYER_SIZE;

    // --- 4. Beräkna var HITBOXEN är (för den gula ramen) ---
    dst_hitbox.x = (int)(player->Hitbox.x - cam->x);
    dst_hitbox.y = (int)(player->Hitbox.y - cam->y);
    dst_hitbox.w = (int)player->Hitbox.w;
    dst_hitbox.h = (int)player->Hitbox.h;

    // --- 5. Rita ---
    SDL_RenderCopy(renderer, texture, &src, &dst_render);

    SDL_GetRenderDrawColor(renderer, &old_r, &old_g, &old_b, &old_a);
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &dst_hitbox);
    SDL_SetRenderDrawColor(renderer, old_r, old_g, old_b, old_a);
}