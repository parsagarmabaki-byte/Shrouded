#include "lobby.h"
#include <stdio.h>

int initiate(waitForPlayers *pWait)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL_Init: %s\n", SDL_GetError());
        return 0;
    }
    if (TTF_Init() < 0)
    {
        printf("TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        printf("IMG_Init: %s\n", IMG_GetError());
        return 0;
    }

    pWait->window = SDL_CreateWindow(
        "Shrouded Lobby",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_FULLSCREEN_DESKTOP);

    if (!pWait->window)
    {
        printf("SDL_CreateWindow: %s\n", SDL_GetError());
        IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 0;
    }

    pWait->renderer = SDL_CreateRenderer(pWait->window, -1, SDL_RENDERER_ACCELERATED);
    if (!pWait->renderer)
    {
        printf("SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(pWait->window);
        IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 0;
    }

    pWait->Font = TTF_OpenFont("assets/fonts/BebasNeue-Regular.ttf", 60);
    if (!pWait->Font)
    {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_DestroyRenderer(pWait->renderer);
        SDL_DestroyWindow(pWait->window);
        IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 0;
    }

    SDL_Surface *bgSurface = IMG_Load("assets/lobbyscreen/waitingforplayers.png");
    if (!bgSurface)
    {
        printf("IMG_Load: %s\n", IMG_GetError());
        TTF_CloseFont(pWait->Font);
        SDL_DestroyRenderer(pWait->renderer);
        SDL_DestroyWindow(pWait->window);
        IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 0;
    }
    pWait->background = SDL_CreateTextureFromSurface(pWait->renderer, bgSurface);
    SDL_FreeSurface(bgSurface);

    if (!pWait->background)
    {
        printf("SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
        TTF_CloseFont(pWait->Font);
        SDL_DestroyRenderer(pWait->renderer);
        SDL_DestroyWindow(pWait->window);
        IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 0;
    }
    return 1;
}

void cleanLobby(waitForPlayers *pWait)
{
    if (pWait->Font) TTF_CloseFont(pWait->Font);
    if (pWait->background) SDL_DestroyTexture(pWait->background);
    if (pWait->renderer) SDL_DestroyRenderer(pWait->renderer);
    if (pWait->window) SDL_DestroyWindow(pWait->window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

int countActivePlayers(gameState *state)
{
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
        if (state->players[i].active) count++;
    return count;
}

void renderWaitingScreen(waitForPlayers *pWait, gameState *state)
{
    SDL_Color white = {255, 255, 255, 255};
    SDL_RenderCopy(pWait->renderer, pWait->background, NULL, NULL);

    int connectedPlayers = countActivePlayers(state);
    char text[64];
    snprintf(text, sizeof(text), "%d/%d CONNECTED", connectedPlayers, MAX_PLAYERS);

    SDL_Surface *surface = TTF_RenderText_Blended(pWait->Font, text, white);
    if (!surface) return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(pWait->renderer, surface);
    if (!texture) { SDL_FreeSurface(surface); return; }

    SDL_Rect dst;
    dst.w = surface->w;
    dst.h = surface->h;

    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(pWait->renderer, &windowWidth, &windowHeight);
    dst.x = (windowWidth - dst.w) / 2;
    dst.y = windowHeight / 6;
    SDL_FreeSurface(surface);

    SDL_RenderCopy(pWait->renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);

    if (connectedPlayers >= 1)
    {
        SDL_Surface *startSurface = TTF_RenderText_Blended(pWait->Font, "PRESS SPACE TO START", white);
        if (startSurface)
        {
            SDL_Texture *startTexture = SDL_CreateTextureFromSurface(pWait->renderer, startSurface);
            if (startTexture)
            {
                SDL_Rect startRect;
                startRect.w = startSurface->w;
                startRect.h = startSurface->h;
                startRect.x = (windowWidth - startRect.w) / 2;
                startRect.y = windowHeight - 180;
                SDL_RenderCopy(pWait->renderer, startTexture, NULL, &startRect);
                SDL_DestroyTexture(startTexture);
            }
            SDL_FreeSurface(startSurface);
        }
    }
    SDL_RenderPresent(pWait->renderer);
}
