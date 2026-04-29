#include "text.h"
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>

struct Text {
    SDL_Renderer *renderare;
    TTF_Font     *typsnitt;
    SDL_Texture  *textur;
    int           bredd;
    int           hojd;
};

bool text_initiera(void) {
    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init misslyckades: %s", TTF_GetError());
        return false;
    }
    return true;
}

void text_avsluta(void) {
    TTF_Quit();
}

// Uppdaterar texten som ska visas. Skapar ny texture internt.
Text text_skapa(SDL_Renderer *renderare, const char *typsnittsvag, int storlek) {
    Text t = malloc(sizeof(struct Text));
    if (!t) return NULL;

    t->renderare = renderare;
    t->textur    = NULL;
    t->bredd     = 0;
    t->hojd      = 0;

    t->typsnitt = TTF_OpenFont(typsnittsvag, storlek);
    if (!t->typsnitt) {
        SDL_Log("TTF_OpenFont misslyckades: %s", TTF_GetError());
        free(t);
        return NULL;
    }
    return t;
}

bool text_satt(Text t, const char *innehall, SDL_Color farg) {
    if (t->textur) {
        SDL_DestroyTexture(t->textur);
        t->textur = NULL;
    }

    SDL_Surface *yta = TTF_RenderUTF8_Blended(t->typsnitt, innehall, farg);
    if (!yta) {
        SDL_Log("TTF_RenderUTF8_Blended misslyckades: %s", TTF_GetError());
        return false;
    }

    t->textur = SDL_CreateTextureFromSurface(t->renderare, yta);
    t->bredd  = yta->w;
    t->hojd   = yta->h;
    SDL_FreeSurface(yta);

    return t->textur != NULL;
}

void text_rita(Text t, int x, int y) {
    if (!t->textur) return;
    SDL_Rect dest = {
        x - t->bredd / 2,
        y - t->hojd / 2,
        t->bredd,
        t->hojd
    };
    SDL_RenderCopy(t->renderare, t->textur, NULL, &dest);
}

void text_rita_vid(Text t, int x, int y) {
    if (!t->textur) return;
    SDL_Rect dest = { x, y, t->bredd, t->hojd };
    SDL_RenderCopy(t->renderare, t->textur, NULL, &dest);
}

int text_hamta_bredd(Text t)  { return t->bredd; }
int text_hamta_hojd(Text t)   { return t->hojd; }

void text_forstor(Text t) {
    if (!t) return;
    if (t->textur)   SDL_DestroyTexture(t->textur);
    if (t->typsnitt) TTF_CloseFont(t->typsnitt);
    free(t);
}