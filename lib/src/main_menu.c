#include "main_menu.h"
#include "text.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

// ── Knapp-layout ────────────────────────────────────────
#define BTN_W 360
#define BTN_H 90
#define BTN_GAP 30           // vertikalt avstånd mellan knappar

// Hjälpfunktion: kolla om en punkt ligger inuti en SDL_Rect.
static int point_in_rect(int x, int y, const SDL_Rect *r)
{
    return x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

// Rita en knapp med fyllning, kantlinje och centrerad text.
// `hovered` styr färgen så användaren ser vilken knapp som är aktiv.
static void draw_button(SDL_Renderer *renderer, Text label, const SDL_Rect *rect, int hovered)
{
    if (hovered)
    {
        SDL_SetRenderDrawColor(renderer, 240, 200, 90, 255);   // bright gold
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 180, 140, 50, 220);   // darker gold
    }

    SDL_RenderFillRect(renderer, rect);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, rect);

    // Centrerad text i knappen
    int cx = rect->x + rect->w / 2;
    int cy = rect->y + rect->h / 2;
    text_draw(label, cx, cy);
}

int showMainMenu(waitForPlayers *pWait)
{
    SDL_Event event;
    SDL_Color white  = {255, 255, 255, 255};

    // Försök ladda menyspecifik bakgrund. Om det misslyckas använder vi
    // lobby-bakgrunden istället så menyn fortfarande fungerar.
    SDL_Texture *menuBackground = NULL;
    SDL_Surface *bgSurface = IMG_Load("assets/lobbyscreen/mainmenu.png");
    if (bgSurface)
    {
        menuBackground = SDL_CreateTextureFromSurface(pWait->renderer, bgSurface);
        SDL_FreeSurface(bgSurface);
    }
    else
    {
        printf("IMG_Load (mainmenu.png): %s — använder lobby-bakgrund\n", IMG_GetError());
    }
    SDL_Texture *backgroundToDraw = menuBackground ? menuBackground : pWait->background;

    // Skapa lokala texter för menyn (städas upp innan vi returnerar)
    const char *fontPath = "assets/fonts/BebasNeue-Regular.ttf";

    Text startText = text_create(pWait->renderer, fontPath, 60);
    Text exitText  = text_create(pWait->renderer, fontPath, 60);
    Text hintText  = text_create(pWait->renderer, fontPath, 22);

    text_set(startText, "START", white);
    text_set(exitText,  "EXIT", white);
    text_set(hintText,  "Made by: Alexander, Alvin, Gergo, Hedi, Parsa och Marcus",white);

    // Spelaren styr menyn med musen. Esc fungerar alltid som genväg
    // för att stänga menyn (samma som Exit-knappen).
    int finished = 0;
    int result = 0;

    SDL_SetRenderDrawBlendMode(pWait->renderer, SDL_BLENDMODE_BLEND);

    while (!finished)
    {
        // ── Layout: räknas om varje frame ifall fönsterstorlek ändras ──
        int windowWidth, windowHeight;
        SDL_GetRendererOutputSize(pWait->renderer, &windowWidth, &windowHeight);

        int centerX = windowWidth / 2;
        int centerY = windowHeight / 2;

        SDL_Rect startRect = {
            centerX - BTN_W / 2,
            centerY - BTN_H - BTN_GAP / 2,
            BTN_W,
            BTN_H
        };
        SDL_Rect exitRect = {
            centerX - BTN_W / 2,
            centerY + BTN_GAP / 2,
            BTN_W,
            BTN_H
        };

        // ── Mus-position för hover-detektion ──
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

            // Endast Esc lyssnas på från tangentbordet — fungerar som
            // snabb genväg för att stänga menyn.
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
        SDL_RenderCopy(pWait->renderer, backgroundToDraw, NULL, NULL);

        // Lätt mörk overlay så texten syns tydligt även mot ljusa bakgrunder.
        // Sänk alpha (3:e arg från sista) om bilden ska synas mer, eller ta
        // bort dessa två rader helt om bilden redan har bra kontrast.
        SDL_SetRenderDrawColor(pWait->renderer, 0, 0, 0, 80);
        SDL_RenderFillRect(pWait->renderer, NULL);

        // Knappar — markera den som musen är över
        draw_button(pWait->renderer, startText, &startRect, hoverStart);
        draw_button(pWait->renderer, exitText,  &exitRect,  hoverExit);

        // Hjälptext längst ner
        text_draw(hintText, centerX, windowHeight - 60);

        SDL_RenderPresent(pWait->renderer);
    }

    text_destroy(startText);
    text_destroy(exitText);
    text_destroy(hintText);
    if (menuBackground) SDL_DestroyTexture(menuBackground);

    return result;
}