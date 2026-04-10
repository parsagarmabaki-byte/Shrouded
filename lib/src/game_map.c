void loading_img(SDL_Window *window, SDL_Renderer *renderer, int window_width, int window_height)
{
    SDL_Surface *surface = IMG_Load("assets/images/Game_map.png");
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    SDL_Rect destRect = {150, 50, window_width, window_height};

    SDL_RenderCopy(renderer, texture, NULL, &destRect);
    SDL_RenderPresent(renderer);
}

