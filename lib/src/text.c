#include "text.h"
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>

struct Text {
    SDL_Renderer *renderer;
    TTF_Font     *font;
    SDL_Texture  *texture;
    int           width;
    int           height;
};

bool text_init(void) {
    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
        return false;
    }
    return true;
}

void text_quit(void) {
    TTF_Quit();
}

// Uppdaterar texten som ska visas. Skapar ny texture internt.
Text text_create(SDL_Renderer *renderer, const char *font_path, int size) {
    Text t = malloc(sizeof(struct Text));
    if (!t) return NULL;

    t->renderer = renderer;
    t->texture    = NULL;
    t->width     = 0;
    t->height      = 0;

    t->font = TTF_OpenFont(font_path, size);
    if (!t->font) {
        SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
        free(t);
        return NULL;
    }
    return t;
}

bool text_set(Text t, const char *content, SDL_Color color) {
    if (t->texture) {
        SDL_DestroyTexture(t->texture);
        t->texture = NULL;
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(t->font, content, color);
    if (!surface) {
        SDL_Log("TTF_RenderUTF8_Blended failed: %s", TTF_GetError());
        return false;
    }

    t->texture = SDL_CreateTextureFromSurface(t->renderer, surface);
    t->width  = surface->w;
    t->height   = surface->h;
    SDL_FreeSurface(surface);

    return t->texture != NULL;
}

void text_draw(Text t, int x, int y) {
    if (!t->texture) return;
    SDL_Rect dest = {
        x - t->width / 2,
        y - t->height / 2,
        t->width,
        t->height
    };
    SDL_RenderCopy(t->renderer, t->texture, NULL, &dest);
}

void text_draw_at(Text t, int x, int y) {
    if (!t->texture) return;
    SDL_Rect dest = { x, y, t->width, t->height };
    SDL_RenderCopy(t->renderer, t->texture, NULL, &dest);
}

int text_get_width(Text t)  { return t->width; }
int text_get_height(Text t)   { return t->height; }

void text_destroy(Text t) {
    if (!t) return;
    if (t->texture)   SDL_DestroyTexture(t->texture);
    if (t->font) TTF_CloseFont(t->font);
    free(t);
}