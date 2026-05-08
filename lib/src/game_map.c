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

    return texture;
}

GameAssets load_assets(SDL_Renderer *renderer)
{
    GameAssets asset = {0};
    asset.map_texture = loading_img(renderer,"assets/images/Game_map.png");
    asset.vignette_img = loading_img(renderer, "assets/images/vignette2.png");
    if (asset.vignette_img)
        SDL_SetTextureBlendMode(asset.vignette_img, SDL_BLENDMODE_BLEND);

    asset.innocent_img = loading_img(renderer, "assets/images/innocent.png");
    asset.killer_img   = loading_img(renderer, "assets/images/killer.png"); 
    asset.role_art_img = loading_img(renderer, "assets/images/show_role.png");

    asset.kill_button_active = loading_img(renderer, "assets/images/kill_button_active.png");
    asset.kill_button_deactive = loading_img(renderer, "assets/images/kill_button_deactive.png");

    asset.report_button_deactive = loading_img(renderer, "assets/images/report_button_deactive.png");
    asset.report_button_active = loading_img(renderer, "assets/images/report_button_active.png");

    asset.emergency_button_view = loading_img(renderer, "assets/images/emergency_button_view.png");
    asset.emergency_button_hover = loading_img(renderer, "assets/images/Emergencybutton_light_up.png");
    asset.dead_body_reported_info = loading_img(renderer, "assets/images/body_reported.png");
    asset.emergency_meeting_info = loading_img(renderer, "assets/images/Emergency_meeting_info.png");
    asset.emergency_meeting_alive = loading_img(renderer, "assets/images/emergency_meeting_alive.png");
    asset.emergency_meeting_dead = loading_img(renderer, "assets/images/emergency_meeting_dead.png");
    asset.emergency_meeting_icon = loading_img(renderer, "assets/images/meeting-ikon.png");

    asset.skip_vote_banner = loading_img(renderer, "assets/voting_result_assets/Skip_banner.png");
    asset.no_one_eliminated = loading_img(renderer, "assets/voting_result_assets/No_one_eliminated.png");
    asset.crewmates_win_screen = loading_img(renderer, "assets/win_screens/CREWMATES_WINS.png");
    

    // Pausmeny-bilder
    asset.pause_bg     = loading_img(renderer, "assets/Images/quit.png");
    asset.pause_resume = loading_img(renderer, "assets/Images/resume_quit.png");
    asset.pause_exit   = loading_img(renderer, "assets/Images/exit_quit.png");

    if (!asset.emergency_meeting_info)
        printf("image not loaded");

    char path[64];
    const char *killer_win_screen_paths[PLAYER_SLOTS] = {
        "assets/win_screens/GREEN_KILLER_WINS.png",
        "assets/win_screens/RED_KILLER_WINS.png",
        "assets/win_screens/BLUE_KILLER_WINS.png",
        "assets/win_screens/PURPLE_KILLER_WINS.png",
        "assets/win_screens/YELLOW_KILLER_WINS.png",
        "assets/win_screens/BLACK_KILLER_WINS.png"
    };

    for (int i = 0; i < PLAYER_SLOTS; i++)
    {
        snprintf(path, sizeof(path), "assets/sprites/skin%d.png", i);
        asset.skins[i] = loading_img(renderer, path);
        
        snprintf(path, sizeof(path), "assets/sprites/dead_skin%d.png", i);
        asset.dead_skins[i] = loading_img(renderer, path);
        
        snprintf(path, sizeof(path), "assets/images/player%d_alive.png", i+1);
        asset.players_alive_banner[i] = loading_img(renderer, path);
        
        snprintf(path, sizeof(path), "assets/images/player%d_dead_transparent.png", i+1);
        asset.players_dead_banner[i] = loading_img(renderer, path);

        snprintf(path, sizeof(path), "assets/voting_result_assets/Player%d_banner_dead.png", i+1);
        asset.players_voting_result_dead[i] = loading_img(renderer, path);

        snprintf(path, sizeof(path), "assets/voting_result_assets/Player%d_banner.png", i+1);
        asset.players_voting_result_alive[i] = loading_img(renderer, path);

        snprintf(path, sizeof(path), "assets/voting_result_assets/Player%d_Eliminated.png", i+1);
        asset.players_kicked_out[i] = loading_img(renderer, path);

        asset.killer_win_screens[i] = loading_img(renderer, killer_win_screen_paths[i]);
    }
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

void destroy_assets(GameAssets *assets)
{
    if (!assets) return;

    for (int i = 0; i < PLAYER_SLOTS; i++)
    {
        destroy_texture(&assets->skins[i]);
        destroy_texture(&assets->dead_skins[i]);
        destroy_texture(&assets->killer_win_screens[i]);
    }

    destroy_texture(&assets->map_texture);
    destroy_texture(&assets->vignette_img);
    destroy_texture(&assets->killer_img);
    destroy_texture(&assets->innocent_img);
    destroy_texture(&assets->role_art_img);
    destroy_texture(&assets->kill_button_active);
    destroy_texture(&assets->kill_button_deactive);
    destroy_texture(&assets->report_button_deactive);
    destroy_texture(&assets->report_button_active);
    destroy_texture(&assets->emergency_button_view);
    destroy_texture(&assets->emergency_meeting_info);
    destroy_texture(&assets->dead_body_reported_info);
    destroy_texture(&assets->emergency_meeting_alive);
    destroy_texture(&assets->crewmates_win_screen);
    destroy_texture(&assets->pause_bg);
    destroy_texture(&assets->pause_resume);
    destroy_texture(&assets->pause_exit);
    destroy_texture(&assets->emergency_button_hover);
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
