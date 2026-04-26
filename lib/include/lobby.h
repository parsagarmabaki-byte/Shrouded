#ifndef LOBBY_H_INCLUDED
#define LOBBY_H_INCLUDED

#include <stddef.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "network_data.h"

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *Font;
    SDL_Texture *background;
} waitForPlayers;

int initiate(waitForPlayers *pWait);
void cleanLobby(waitForPlayers *pWait);
int countActivePlayers(gameState *state);
int promptServerAddress(waitForPlayers *pWait, char *buffer, size_t buffer_size, const char *error_message);
void renderWaitingScreen(waitForPlayers *pWait, gameState *state);

#endif
