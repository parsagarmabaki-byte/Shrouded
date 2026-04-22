#include "game_map.h"

SDL_Texture *loading_img(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *surface = IMG_Load(path);
    if (!surface)
    {
        printf("IMG_Load failed: %s\n", IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture)
    {
        printf("SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
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
    asset.kill_button_img = loading_img(renderer, "assets/images/Kill_button.png");
    asset.killer_img   = loading_img(renderer, "assets/images/killer.png"); 
    asset.role_art_img = loading_img(renderer, "assets/images/show_role.png");

    char path[64];
    for (int i = 0; i < PLAYER_SLOTS; i++)
    {
        snprintf(path, sizeof(path), "assets/sprites/skin%d.png", i);
        asset.skins[i] = loading_img(renderer, path);
    }
    
    for (int i = 0; i < PLAYER_SLOTS; i++)
    {
        snprintf(path, sizeof(path), "assets/sprites/dead_skin%d.png", i);
        asset.dead_skins[i] = loading_img(renderer, path);
    }
    return asset;   
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
    
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, background_img, NULL, &picturesize);
}
