#include "emergency_meeting.h"
#include "game_map.h"
#include "client_network.h"

void emergency_meeting_view(SDL_Renderer *renderer, SDL_Texture *emergency_button_view, SDL_Texture *emergency_button_hover)
{
    int width = 680;
    int height = 455;
    SDL_Rect picture_size = {(LOGICAL_SCREEN_WIDTH / 2) - width / 2, ((LOGICAL_SCREEN_HEIGHT) / 2) - height / 2, width, height};
    SDL_RenderCopy(renderer, emergency_button_view, NULL, &picture_size);

    // Rendera hover-bilden ovanpå knappen om musen hovrar
    SDL_Rect emergency_button = {(LOGICAL_SCREEN_WIDTH / 2) - 17, ((LOGICAL_SCREEN_HEIGHT) / 2) - 59, 26, 26};
    if (is_hovering(renderer, emergency_button))
    {
        SDL_RenderCopy(renderer, emergency_button_hover, NULL, &emergency_button);
    }
}

void render_emergency_meeting(SDL_Renderer *renderer, GameAssets assets, GameState *state, int id_reported, int targeted_banner_id, Text timer_meeting_text, int *player_voted)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    render_emergency_map(renderer, assets, state->players[state->local_player_id].isAlive, *player_voted);
    render_banners(renderer, assets, state, targeted_banner_id, *player_voted);
    render_emergency_icon(renderer, assets.emergency_meeting_icon, id_reported);
    SDL_Rect submit_button = {500, 800, 200, 200};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &submit_button);

    // --- Timer ---
    int seconds_left = (state->meeting_time_remaining + 999) / 1000;
    char timer_buf[16];
    snprintf(timer_buf, sizeof(timer_buf), "%d", seconds_left);

    SDL_Color white = {255, 255, 255, 255};
    text_set(timer_meeting_text, timer_buf, white);
    text_draw(timer_meeting_text, LOGICAL_SCREEN_WIDTH / 2, 600);
}

int target_player_banner(SDL_Renderer *renderer, GameState state, SDL_Event *event, int player_alive, int target_banner_id)
{
    if (event->type == SDL_MOUSEBUTTONDOWN && player_alive)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (!state.players[i].isAlive)
                continue;

            if (is_hovering(renderer, get_banner_rect(i)))
            {
                if (target_banner_id == i)
                    return -1; // klickade samma igen = avmarkera

                return i; // klickade ny banner = välj den
            }
        }
    }
    return target_banner_id;
}

int handle_send_vote_button(Client *client, SDL_Renderer *renderer, SDL_Event *event, int player_alive, int targeted_banner, int *player_voted, int voter_id)
{
    SDL_Rect submit_button = {VOTE_SUBMIT_X, VOTE_SUBMIT_Y, VOTE_SUBMIT_W, VOTE_SUBMIT_H};
    SDL_Rect skip_button = {VOTE_SKIP_X, VOTE_SKIP_Y, VOTE_SKIP_W, VOTE_SKIP_H};
    if (event->type == SDL_MOUSEBUTTONDOWN && player_alive && *player_voted == -1)
    {
        if (is_hovering(renderer, submit_button))
        {
            send_vote(client, targeted_banner, voter_id);
            *player_voted = 1;
        }
        else if (is_hovering(renderer, skip_button))
        {
            send_vote(client, -1, voter_id);
            *player_voted = 1;
        }
    }
    return 0;
}

void render_emergency_map(SDL_Renderer *renderer, GameAssets assets, int player_alive, int player_voted)
{
    SDL_Rect submit_button = {VOTE_SUBMIT_X, VOTE_SUBMIT_Y, VOTE_SUBMIT_W, VOTE_SUBMIT_H};
    SDL_Rect skip_button = {VOTE_SKIP_X, VOTE_SKIP_Y, VOTE_SKIP_W, VOTE_SKIP_H};
    SDL_Texture *map_texture;
    if (player_alive && player_voted == -1)
    {
        if (is_hovering(renderer, submit_button))
            map_texture = assets.emergency_meeting_submit;
        else if (is_hovering(renderer, skip_button))
            map_texture = assets.emergency_meeting_skip;
        else
            map_texture = assets.emergency_meeting_alive;
    }
    else
        map_texture = assets.emergency_meeting_dead;
    SDL_RenderCopy(renderer, map_texture, NULL, NULL);
}

void render_banners(SDL_Renderer *renderer, GameAssets assets, GameState *state, int targeted_banner_id, int player_voted)
{
    SDL_Rect banner;
    SDL_Texture *banner_img;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        banner = get_banner_rect(i);

        if (!state->players[i].isAlive)
        {
            SDL_RenderCopy(renderer, assets.players_dead_banner[i], NULL, &banner);
            continue;
        }

        int has_target = (targeted_banner_id != -1);
        int is_targeted = (targeted_banner_id == i);

        // Hover ska bara komma från musen om ingen target finns.
        // Om target finns, ska bara targetad banner få hover-look.
        int hovered = 0;
        if (!has_target)
        {
            hovered = is_hovering(renderer, banner);
        }

        if (hovered || is_targeted || (player_voted && is_targeted))
        {
            banner.h += 14;
            banner.y -= 12;
            banner.x -= 2;
            banner_img = assets.players_alive_banner_hover[i];
        }
        else
        {
            banner_img = assets.players_alive_banner[i];
        }

        // Dimma andra banners när en target finns
        if (has_target && !is_targeted)
        {
            SDL_SetTextureAlphaMod(banner_img, 95);
            SDL_SetTextureColorMod(banner_img, 130, 130, 130);
        }
        else
        {
            SDL_SetTextureAlphaMod(banner_img, 255);
            SDL_SetTextureColorMod(banner_img, 255, 255, 255);
        }

        SDL_RenderCopy(renderer, banner_img, NULL, &banner);

        SDL_SetTextureAlphaMod(banner_img, 255);
        SDL_SetTextureColorMod(banner_img, 255, 255, 255);
    }
}

SDL_Rect get_banner_rect(int i)
{
    int start_x = 240;
    int start_y = 170;
    int col = i % 2;
    int row = i / 2;

    SDL_Rect banner = {
        start_x + col * col_dx,
        start_y + row * row_dy,
        350,
        109};
    return banner;
}

void render_emergency_icon(SDL_Renderer *renderer, SDL_Texture *icon, int id_reported)
{
    int start_x = 520;
    int start_y = 200;

    if (id_reported < 0 || id_reported >= 6)
        return;

    int col = id_reported % 2;
    int row = id_reported / 2;

    int x_pos = start_x + col * col_dx;
    int y_pos = start_y + row * row_dy;
    SDL_Rect icon_pos = {x_pos, y_pos, 50, 50};
    SDL_RenderCopy(renderer, icon, NULL, &icon_pos);
}

void render_voting_screen(SDL_Renderer *renderer, GameState *state, GameAssets assets, int voting_result)
{
    render_voting_result_layer(renderer, assets, voting_result);
    render_voting_banners(renderer, state, assets);
    render_voting_results(renderer, state, state->voting_results);
}

void render_voting_result_layer(SDL_Renderer *renderer, GameAssets assets, int target_id)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Texture *layer = NULL;
    if (target_id != -1)
        layer = assets.players_kicked_out[target_id];
    else
        layer = assets.no_one_eliminated;
    SDL_RenderCopy(renderer, layer, NULL, NULL);
}

void render_voting_banners(SDL_Renderer *renderer, GameState *state, GameAssets assets)
{
    SDL_Texture *banner;
    SDL_Rect banner_size = {220, 450, 116, 195};

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (i > 0)
            banner_size.x += 125;
        bool player_alive = state->players[i].isAlive;
        if (player_alive)
            banner = assets.players_voting_result_alive[i];
        else
            banner = assets.players_voting_result_dead[i];

        SDL_RenderCopy(renderer, banner, NULL, &banner_size);
    }

    banner_size.x += 125;
    SDL_RenderCopy(renderer, assets.skip_vote_banner, NULL, &banner_size);
    // SDL_RenderCopy(renderer, assets.players_voting_result_alive[0], NULL, &banner_size);
}

void render_voting_results(SDL_Renderer *renderer, GameState *state, int voting_results[MAX_PLAYERS])
{
    Text Font_texture = text_create(renderer, "assets/fonts/Cinzel_Decorative/CinzelDecorative-Bold.ttf", 32);
    int x_postion = 153;
    SDL_Color color = {0, 0, 0, 255};
    char buffer[4];

    // SDL_RenderFillRect(renderer, &font_size);
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        x_postion += 125;
        if (state->players[i].isAlive)
        {
            snprintf(buffer, sizeof(buffer), "%d", voting_results[i]);
            text_set(Font_texture, buffer, color);
            text_draw(Font_texture, x_postion, 597);
        };
    }
    x_postion += 125;
    snprintf(buffer, sizeof(buffer), "%d", voting_results[MAX_PLAYERS]);
    text_set(Font_texture, buffer, color);
    text_draw(Font_texture, x_postion, 597);
    text_destroy(Font_texture);

}
