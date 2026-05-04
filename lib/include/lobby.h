#ifndef LOBBY_H_INCLUDED
#define LOBBY_H_INCLUDED

#include <stddef.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "network_data.h"
#include "text.h"

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *background;

    // Statiska texter (sätts en gång i initiate)
    Text titleText;      // "CONNECT TO SERVER"
    Text subtitleText;   // "Enter the server IP address below"
    Text enterText;      // "Press Enter to connect"
    Text escText;        // "Esc closes the client"
    Text startText;      // "PRESS SPACE TO START"

    // Dynamiska texter (uppdateras vid behov)
    Text inputText;      // IP-adressen som användaren skriver
    Text errorText;      // Felmeddelande vid misslyckad anslutning
    Text connectedText;  // "2/6 CONNECTED"
} waitForPlayers;

int initiate(waitForPlayers *pWait);
void cleanLobby(waitForPlayers *pWait);
int countActivePlayers(gameState *state);
int promptServerAddress(waitForPlayers *pWait, char *buffer, size_t buffer_size, const char *error_message);
void renderWaitingScreen(waitForPlayers *pWait, gameState *state);

#endif