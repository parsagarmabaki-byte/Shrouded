#include "emergency_meeting.h"
#include "game_map.h"
#include "client_network.h"

void emergency_meeting_view(SDL_Renderer *renderer, SDL_Texture *emergency_button_view)
{
    int width = 685;
    int height = 460;
    SDL_Rect picture_size = {(LOGICAL_SCREEN_WIDTH / 2) - width / 2, ((LOGICAL_SCREEN_HEIGHT) / 2) - height / 2, width, height};
    SDL_RenderCopy(renderer, emergency_button_view, NULL, &picture_size);
}

void render_emergency_meeting(SDL_Renderer *renderer, GameAssets assets, gameState *state, int id_reported, int targeted_banner_id)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    render_emergency_map(renderer, assets, state->players[state->local_player_id].isAlive);
    render_banners(renderer, assets, state, targeted_banner_id);
    render_emergency_icon(renderer, assets.emergency_meeting_icon, id_reported);
    SDL_Rect submit_button = {500, 800, 200, 200};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &submit_button);
}

int target_player_banner(SDL_Renderer *renderer, gameState state, SDL_Event *event, int player_alive, int target_banner_id)
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

int handle_send_vote_button(Client *client, SDL_Renderer *renderer, SDL_Event *event, int player_alive, int targeted_banner)
{
    SDL_Rect submit_button = {260, 555, 265, 75};
    SDL_Rect skip_button = {760, 555, 265, 75};
    if (event->type == SDL_MOUSEBUTTONDOWN && player_alive)
    {
        if (is_hovering(renderer, submit_button))
            send_vote(client, targeted_banner);
        else if (is_hovering(renderer, skip_button))
            send_vote(client, -1);
        // send_player_vote()
    }
}

void render_emergency_map(SDL_Renderer *renderer, GameAssets assets, int player_alive)
{
    SDL_Texture *map_texture;
    if (player_alive)
        map_texture = assets.emergency_meeting_alive;
    else if (!player_alive)
        map_texture = assets.emergency_meeting_dead;
    SDL_RenderCopy(renderer, map_texture, NULL, NULL);
}

void render_banners(SDL_Renderer *renderer, GameAssets assets, gameState *state, int targeted_banner_id)
{
    SDL_Rect banner;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        banner = get_banner_rect(i);

        if (state->players[i].isAlive)
        {
            if (targeted_banner_id != -1 && targeted_banner_id != i)
                SDL_SetTextureAlphaMod(assets.players_alive_banner[i], 155);

            SDL_RenderCopy(renderer, assets.players_alive_banner[i], NULL, &banner);
            SDL_SetTextureAlphaMod(assets.players_alive_banner[i], 255);
        }
        else
        {
            SDL_RenderCopy(renderer, assets.players_dead_banner[i], NULL, &banner);
        }
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

void render_voting_results(SDL_Renderer *renderer, gameState *state, GameAssets assets, int voting_result)
{
    render_voting_result_layer(renderer, assets, voting_result);
    render_voting_result_banners(renderer, state, assets);
}

void render_voting_result_layer(SDL_Renderer *renderer, GameAssets assets, int target_id)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Texture *layer = assets.players_kicked_out[target_id];
    SDL_RenderCopy(renderer, layer, NULL, NULL);
}

void render_voting_result_banners(SDL_Renderer *renderer, gameState *state, GameAssets assets)
{
    SDL_Texture *banner;
    SDL_Rect banner_size = {220,450,116,195};

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (i>0)
            banner_size.x += 125; 
        bool player_alive = state->players[i].isAlive;
        if (player_alive)
            banner = assets.players_voting_result_alive[i];
        else
            banner = assets.players_voting_result_dead[i];
        
        SDL_RenderCopy(renderer,banner,NULL,&banner_size);
    }

    banner_size.x += 125;
    SDL_RenderCopy(renderer,assets.skip_vote_banner,NULL,&banner_size);
     // SDL_RenderCopy(renderer, assets.players_voting_result_alive[0], NULL, &banner_size);
    
}
