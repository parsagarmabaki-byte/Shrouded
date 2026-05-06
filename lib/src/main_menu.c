#include "main_menu.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

// ─────────────────────────────────────────────────────────────────
//  KNAPP-PLACERING I DEFAULT-BILDEN
// ─────────────────────────────────────────────────────────────────
// Procent av fönstrets storlek (0.0 till 1.0) — fungerar oavsett
// fönsterstorlek. Samma rektangel används både för:
//   • Var knapp-on-bilden ska ritas (dst-rect)
//   • Var musen "räknas som över" knappen (hit-zon)
//
// Värdena är kalibrerade för mainmenu_default.png. Om du byter
// bakgrundsbild behöver de mätas om.

// Start-knappen
#define START_X_PCT 0.29f
#define START_Y_PCT 0.61f
#define START_W_PCT 0.341f
#define START_H_PCT 0.092f

// Exit-knappen
#define EXIT_X_PCT  0.29f
#define EXIT_Y_PCT  0.745f
#define EXIT_W_PCT  0.341f
#define EXIT_H_PCT  0.092f

// ─────────────────────────────────────────────────────────────────

// Kolla om en punkt ligger inuti en SDL_Rect.
static int point_in_rect(int x, int y, const SDL_Rect *r)
{
    return x >= r->x && x < r->x + r->w &&
           y >= r->y && y < r->y + r->h;
}

// Räkna ut en SDL_Rect från procent-värden + en referensstorlek.
static SDL_Rect rect_from_pct(float xPct, float yPct, float wPct, float hPct,
                              int refW, int refH)
{
    SDL_Rect r = {
        (int)(xPct * refW),
        (int)(yPct * refH),
        (int)(wPct * refW),
        (int)(hPct * refH)
    };
    return r;
}

// Ladda en bildfil till en SDL_Texture. Returnerar NULL vid fel.
static SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *surface = IMG_Load(path);
    if (!surface)
    {
        printf("IMG_Load (%s): %s\n", path, IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture)
    {
        printf("SDL_CreateTextureFromSurface (%s): %s\n", path, SDL_GetError());
    }
    return texture;
}

int showMainMenu(waitForPlayers *pWait)
{
    SDL_Event event;

    // ── Ladda bakgrund + två "lit" knapp-bilder ──
    SDL_Texture *bgDefault   = load_texture(pWait->renderer, "assets/lobbyscreen/mainmenu_default.png");
    SDL_Texture *startButton = load_texture(pWait->renderer, "assets/lobbyscreen/start_button_on.png");
    SDL_Texture *exitButton  = load_texture(pWait->renderer, "assets/lobbyscreen/exit_button_on.png");

    if (!bgDefault || !startButton || !exitButton)
    {
        printf("Kunde inte ladda en eller flera menybilder — avbryter menyn.\n");
        if (bgDefault)   SDL_DestroyTexture(bgDefault);
        if (startButton) SDL_DestroyTexture(startButton);
        if (exitButton)  SDL_DestroyTexture(exitButton);
        return 0;
    }

    int finished = 0;
    int result = 0;

    while (!finished)
    {
        // ── Räkna ut knapp-rektangeln baserat på fönsterstorlek varje frame ──
        int windowWidth, windowHeight;
        SDL_GetRendererOutputSize(pWait->renderer, &windowWidth, &windowHeight);

        SDL_Rect startRect = rect_from_pct(START_X_PCT, START_Y_PCT,
                                           START_W_PCT, START_H_PCT,
                                           windowWidth, windowHeight);
        SDL_Rect exitRect  = rect_from_pct(EXIT_X_PCT, EXIT_Y_PCT,
                                           EXIT_W_PCT, EXIT_H_PCT,
                                           windowWidth, windowHeight);

        // ── Var är musen? ──
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        int hoverStart = point_in_rect(mx, my, &startRect);
        int hoverExit  = point_in_rect(mx, my, &exitRect);

        // ── Events ──
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                result = 0;
                finished = 1;
                break;
            }

            if (event.type == SDL_KEYDOWN &&
                event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            {
                result = 0;
                finished = 1;
                break;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT)
            {
                if (point_in_rect(event.button.x, event.button.y, &startRect))
                {
                    result = 1;
                    finished = 1;
                }
                else if (point_in_rect(event.button.x, event.button.y, &exitRect))
                {
                    result = 0;
                    finished = 1;
                }
            }
        }

        if (finished) break;

        // ── Render ──
        // 1. Default-bakgrunden (innehåller "släckta" knappar)
        SDL_RenderCopy(pWait->renderer, bgDefault, NULL, NULL);

        // 2. Om musen är över en knapp: rita "lit"-bilden ovanpå.
        //    NULL som srcRect = använd hela knapp-bilden.
        //    startRect/exitRect som dstRect = lägg den exakt på knappens plats.
        if (hoverStart)
        {
            SDL_RenderCopy(pWait->renderer, startButton, NULL, &startRect);
        }
        else if (hoverExit)
        {
            SDL_RenderCopy(pWait->renderer, exitButton, NULL, &exitRect);
        }

        SDL_RenderPresent(pWait->renderer);
    }

    SDL_DestroyTexture(bgDefault);
    SDL_DestroyTexture(startButton);
    SDL_DestroyTexture(exitButton);

    return result;
}