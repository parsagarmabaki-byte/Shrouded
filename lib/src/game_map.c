#include "game_map.h"

SDL_Texture *loading_img(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *surface = IMG_Load(path);
    if (!surface)
    {
        printf("IMG_Load failed for %s: %s\n", path, IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture)
    {
        printf("SDL_CreateTextureFromSurface failed for %s: %s\n", path, SDL_GetError());
        return NULL;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    return texture;
}

GameAssets load_assets(SDL_Renderer *renderer)
{
    GameAssets asset = {0};

    asset.map_texture = loading_img(renderer, "assets/images/Map_assets/Game_map.png");
    asset.vignette_img = loading_img(renderer, "assets/images/Map_assets/vignette2.png");

    if (asset.vignette_img)
        SDL_SetTextureBlendMode(asset.vignette_img, SDL_BLENDMODE_BLEND);

    asset.kill_button_active = loading_img(renderer, "assets/images/Client_UI/kill_button_active.png");
    asset.kill_button_deactive = loading_img(renderer, "assets/images/Client_UI/kill_button_deactive.png");

    asset.report_button_deactive = loading_img(renderer, "assets/images/Client_UI/report_button_deactive.png");
    asset.report_button_active = loading_img(renderer, "assets/images/Client_UI/report_button_active.png");

    asset.emergency_button_view = loading_img(renderer, "assets/images/Meeting_assets/emergency_button_view.png");
    asset.emergency_button_hover = loading_img(renderer, "assets/images/Meeting_assets/Emergencybutton_light_up.png");

    asset.dead_body_reported_info = loading_img(renderer, "assets/images/Meeting_assets/body_reported.png");
    asset.emergency_meeting_info = loading_img(renderer, "assets/images/Meeting_assets/Emergency_meeting_info.png");
    asset.emergency_meeting_alive = loading_img(renderer, "assets/images/Meeting_assets/emergency_meeting_alive.png");

    asset.emergency_meeting_submit = loading_img(renderer, "assets/images/Meeting_assets/emergency_meeting_submit_hover.png");
    asset.emergency_meeting_skip = loading_img(renderer, "assets/images/Meeting_assets/emergency_meeting_skip_hover.png");

    asset.emergency_meeting_dead = loading_img(renderer, "assets/images/Meeting_assets/emergency_meeting_dead.png");
    asset.emergency_meeting_icon = loading_img(renderer, "assets/images/Meeting_assets/meeting-ikon.png");

    asset.skip_vote_banner = loading_img(renderer, "assets/images/voting_result_assets/Skip_banner.png");
    asset.no_one_eliminated = loading_img(renderer, "assets/images/voting_result_assets/No_one_eliminated.png");

    asset.pause_bg = loading_img(renderer, "assets/images/Client_UI/quit.png");
    asset.pause_resume = loading_img(renderer, "assets/images/Client_UI/resume_quit.png");
    asset.pause_exit = loading_img(renderer, "assets/images/Client_UI/exit_quit.png");

    asset.task_indicator = loading_img(renderer, "assets/images/tasks_assets/marker.png");

    if (!asset.emergency_meeting_info)
        printf("image not loaded\n");

    char path[128];

    const char *killer_win_screen_paths[PLAYER_SLOTS] = {
        "assets/images/win_screens/GREEN_KILLER_WINS.png",
        "assets/images/win_screens/RED_KILLER_WINS.png",
        "assets/images/win_screens/BLUE_KILLER_WINS.png",
        "assets/images/win_screens/PURPLE_KILLER_WINS.png",
        "assets/images/win_screens/YELLOW_KILLER_WINS.png",
        "assets/images/win_screens/BLACK_KILLER_WINS.png"
    };

    const char *crewmates_win_screen_paths[PLAYER_SLOTS] = {
        "assets/images/win_screens/W_OUTGREEN.png",
        "assets/images/win_screens/W_OUTRED.png",
        "assets/images/win_screens/W_OUTBLUE.png",
        "assets/images/win_screens/W_OUTPURPLE.png",
        "assets/images/win_screens/W_OUTYELLOW.png",
        "assets/images/win_screens/W_OUTBLACK.png"
    };

    for (int i = 0; i < PLAYER_SLOTS; i++)
    {
        snprintf(path, sizeof(path), "assets/images/sprites/skin%d.png", i);
        asset.skins[i] = loading_img(renderer, path);

        snprintf(path, sizeof(path), "assets/images/sprites/dead_skin%d.png", i);
        asset.dead_skins[i] = loading_img(renderer, path);

        snprintf(path, sizeof(path), "assets/images/Meeting_assets/player%d_alive.png", i + 1);
        asset.players_alive_banner[i] = loading_img(renderer, path);

        snprintf(path, sizeof(path), "assets/images/Meeting_assets/player%d_alive_hover.png", i + 1);
        asset.players_alive_banner_hover[i] = loading_img(renderer, path);

        snprintf(path, sizeof(path), "assets/images/Meeting_assets/player%d_dead_transparent.png", i + 1);
        asset.players_dead_banner[i] = loading_img(renderer, path);

        snprintf(path, sizeof(path), "assets/images/voting_result_assets/Player%d_banner_dead.png", i + 1);
        asset.players_voting_result_dead[i] = loading_img(renderer, path);

        snprintf(path, sizeof(path), "assets/images/voting_result_assets/Player%d_banner.png", i + 1);
        asset.players_voting_result_alive[i] = loading_img(renderer, path);

        snprintf(path, sizeof(path), "assets/images/voting_result_assets/Player%d_Eliminated.png", i + 1);
        asset.players_kicked_out[i] = loading_img(renderer, path);

        asset.killer_win_screens[i] = loading_img(renderer, killer_win_screen_paths[i]);
        asset.crewmates_win_screens[i] = loading_img(renderer, crewmates_win_screen_paths[i]);
    }

    return asset;
}

Game_Show_Role_asset load_show_role_assets(SDL_Renderer *renderer)
{
    Game_Show_Role_asset asset;
    asset.innocent_img = loading_img(renderer, "assets/images/show_role_assets/innocent.png");
    asset.killer_img   = loading_img(renderer, "assets/images/show_role_assets/killer.png"); 
    asset.role_art_img = loading_img(renderer, "assets/images/show_role_assets/show_role.png");
    return asset;
}

static void destroy_texture(SDL_Texture **texture)
{
    if (texture && *texture)
    {
        SDL_DestroyTexture(*texture);
        *texture = NULL;
    }
}

void destroy_assets(GameAssets *asset, Game_Show_Role_asset *role_asset)
{
    if (!asset)
        return;

    destroy_texture(&asset->map_texture);
    destroy_texture(&asset->vignette_img);

    destroy_texture(&asset->kill_button_active);
    destroy_texture(&asset->kill_button_deactive);

    destroy_texture(&asset->report_button_active);
    destroy_texture(&asset->report_button_deactive);

    destroy_texture(&asset->emergency_button_view);
    destroy_texture(&asset->emergency_button_hover);

    destroy_texture(&asset->dead_body_reported_info);
    destroy_texture(&asset->emergency_meeting_info);
    destroy_texture(&asset->emergency_meeting_alive);

    destroy_texture(&asset->emergency_meeting_submit);
    destroy_texture(&asset->emergency_meeting_skip);

    destroy_texture(&asset->emergency_meeting_dead);
    destroy_texture(&asset->emergency_meeting_icon);

    destroy_texture(&asset->skip_vote_banner);
    destroy_texture(&asset->no_one_eliminated);

    destroy_texture(&asset->pause_bg);
    destroy_texture(&asset->pause_resume);
    destroy_texture(&asset->pause_exit);

    destroy_texture(&asset->task_indicator);

    for (int i = 0; i < PLAYER_SLOTS; i++)
    {
        destroy_texture(&asset->skins[i]);
        destroy_texture(&asset->dead_skins[i]);

        destroy_texture(&asset->players_alive_banner[i]);
        destroy_texture(&asset->players_alive_banner_hover[i]);
        destroy_texture(&asset->players_dead_banner[i]);

        destroy_texture(&asset->players_voting_result_dead[i]);
        destroy_texture(&asset->players_voting_result_alive[i]);

        destroy_texture(&asset->players_kicked_out[i]);

        destroy_texture(&asset->killer_win_screens[i]);
        destroy_texture(&asset->crewmates_win_screens[i]);
    }

    if (role_asset)
    {
        destroy_texture(&role_asset->innocent_img);
        destroy_texture(&role_asset->killer_img);
        destroy_texture(&role_asset->role_art_img);
    }
}

void camera_follow(Camera *cam, float player_x, float player_y, int player_w, int player_h)
{
    cam->x = player_x + player_w / 2 - cam->screen_w / 2;
    cam->y = player_y + player_h / 2 - cam->screen_h / 2;

    // Clamp bara om kartan är större än skärmen
    if (GAME_MAP_WIDTH > cam->screen_w) {
        if (cam->x < 0) cam->x = 0;
        if (cam->x > GAME_MAP_WIDTH - cam->screen_w) cam->x = GAME_MAP_WIDTH - cam->screen_w;
    } else {
        cam->x = -(cam->screen_w - GAME_MAP_WIDTH) / 2.0f; // centrera
    }

    if (GAME_MAP_HEIGHT > cam->screen_h) {
        if (cam->y < 0) cam->y = 0;
        if (cam->y > GAME_MAP_HEIGHT - cam->screen_h) cam->y = GAME_MAP_HEIGHT - cam->screen_h;
    } else {
        cam->y = -(cam->screen_h - GAME_MAP_HEIGHT) / 2.0f; // centrera
    }
}

void render_map(SDL_Renderer *renderer, SDL_Texture *background_img, Camera *cam)
{
    int draw_x = (int)-cam->x;
    int draw_y = (int)-cam->y;

    // Om kartan är mindre än skärmen, centrera den
    if (GAME_MAP_WIDTH < cam->screen_w)
        draw_x = (cam->screen_w - GAME_MAP_WIDTH) / 2;
    if (GAME_MAP_HEIGHT < cam->screen_h)
        draw_y = (cam->screen_h - GAME_MAP_HEIGHT) / 2;

    
    SDL_Rect picturesize = {draw_x, draw_y, GAME_MAP_WIDTH, GAME_MAP_HEIGHT};
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, background_img, NULL, &picturesize);
}
