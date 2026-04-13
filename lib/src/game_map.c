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

void camera_follow(Camera *cam, float player_x, float player_y, float player_w, float player_h)
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
void render_vignette(SDL_Renderer *renderer, SDL_Texture *vignette_img)
{
    SDL_Texture *vignetteTexture;
    
    SDL_Surface *surface = IMG_LOAD("assets/images/vignette.png");
    if (!surface)
    {
        printf("IMG_LOAD error: %s\n", IMG_GetError());
    } else
    
    vignetteTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!vignetteTexture)
    {
        printf("CreateTexture error: %s\n", SDL_GetError());
    }
    
    SDL_SetTextureBlendMode(vignetteTexture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(renderer, vignetteTexture, NULL, NULL);
}