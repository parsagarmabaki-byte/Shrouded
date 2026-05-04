#include "lobby.h"
#include "text.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

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
        TTF_Quit();
        SDL_Quit();
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

    pWait->renderer = SDL_CreateRenderer(
        pWait->window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!pWait->renderer)
    {
        printf("SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(pWait->window);
        IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 0;
    }

    SDL_Surface *bgSurface = IMG_Load("assets/lobbyscreen/waitingforplayers.png");
    if (!bgSurface)
    {
        printf("IMG_Load: %s\n", IMG_GetError());
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
        SDL_DestroyRenderer(pWait->renderer);
        SDL_DestroyWindow(pWait->window);
        IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 0;
    }

    // Skapa alla Text-objekt med samma font och storlek
    const char *fontPath = "assets/fonts/BebasNeue-Regular.ttf";
    const int fontSize = 60;

    pWait->titleText     = text_create(pWait->renderer, fontPath, fontSize);
    pWait->subtitleText  = text_create(pWait->renderer, fontPath, fontSize);
    pWait->enterText     = text_create(pWait->renderer, fontPath, fontSize);
    pWait->escText       = text_create(pWait->renderer, fontPath, fontSize);
    pWait->errorText     = text_create(pWait->renderer, fontPath, fontSize);
    pWait->inputText     = text_create(pWait->renderer, fontPath, fontSize);
    pWait->connectedText = text_create(pWait->renderer, fontPath, fontSize);
    pWait->startText     = text_create(pWait->renderer, fontPath, fontSize);

    if (!pWait->titleText || !pWait->subtitleText || !pWait->enterText ||
        !pWait->escText || !pWait->errorText || !pWait->inputText ||
        !pWait->connectedText || !pWait->startText)
    {
        printf("text_create failed for one or more Text objects\n");
        cleanLobby(pWait);
        return 0;
    }

    // Sätt statiska texter en gång — dessa ändras aldrig
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color muted = {210, 210, 210, 255};

    text_set(pWait->titleText,    "CONNECT TO SERVER",                white);
    text_set(pWait->subtitleText, "Enter the server IP address below", muted);
    text_set(pWait->enterText,    "Press Enter to connect",            muted);
    text_set(pWait->escText,      "Esc closes the client",             muted);
    text_set(pWait->startText,    "PRESS SPACE TO START",              white);

    return 1;
}

void cleanLobby(waitForPlayers *pWait)
{
    text_destroy(pWait->titleText);
    text_destroy(pWait->subtitleText);
    text_destroy(pWait->enterText);
    text_destroy(pWait->escText);
    text_destroy(pWait->errorText);
    text_destroy(pWait->inputText);
    text_destroy(pWait->connectedText);
    text_destroy(pWait->startText);

    if (pWait->background) SDL_DestroyTexture(pWait->background);
    if (pWait->renderer)   SDL_DestroyRenderer(pWait->renderer);
    if (pWait->window)     SDL_DestroyWindow(pWait->window);
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

int promptServerAddress(waitForPlayers *pWait, char *buffer, size_t buffer_size, const char *error_message)
{
    SDL_Event event;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red   = {255, 120, 120, 255};
    int windowWidth, windowHeight;
    int finished = 0;
    int textChanged = 1; // Flagga så vi sätter inputText första framen

    SDL_StartTextInput();

    while (!finished)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                SDL_StopTextInput();
                return 0;
            }

            if (event.type == SDL_TEXTINPUT)
            {
                size_t len = strlen(buffer);
                size_t input_len = strlen(event.text.text);
                int only_valid_chars = 1;

                if (len + input_len < buffer_size)
                {
                    for (size_t i = 0; i < input_len; i++)
                    {
                        char c = event.text.text[i];
                        if (!isdigit(c) && c != '.')
                        {
                            only_valid_chars = 0;
                            break;
                        }
                    }

                    if (only_valid_chars)
                    {
                        strcat(buffer, event.text.text);
                        textChanged = 1;
                    }
                }
            }

            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    SDL_StopTextInput();
                    return 0;
                }

                if (event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE)
                {
                    size_t len = strlen(buffer);
                    if (len > 0)
                    {
                        buffer[len - 1] = '\0';
                        textChanged = 1;
                    }
                }

                if (event.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                    event.key.keysym.scancode == SDL_SCANCODE_KP_ENTER)
                {
                    if (strlen(buffer) > 0)
                    {
                        finished = 1;
                    }
                }
            }
        }

        // Uppdatera inputText bara när texten faktiskt ändrats
        if (textChanged)
        {
            if (strlen(buffer) > 0)
                text_set(pWait->inputText, buffer, white);
            textChanged = 0;
        }

        // Uppdatera felmeddelande (sätts en gång per anrop till promptServerAddress)
        // Görs utanför loopen om det vore statiskt, men error_message kan vara NULL
        // så vi sätter det här en gång (behöver egentligen bara göras en gång,
        // men text_set är billig jämfört med att rita varje frame)

        SDL_RenderCopy(pWait->renderer, pWait->background, NULL, NULL);

        SDL_GetRendererOutputSize(pWait->renderer, &windowWidth, &windowHeight);

        // Panel
        SDL_Rect panel;
        panel.x = windowWidth / 2 - 360;
        panel.y = windowHeight / 2 - 215;
        panel.w = 720;
        panel.h = 430;

        SDL_SetRenderDrawBlendMode(pWait->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(pWait->renderer, 12, 19, 28, 220);
        SDL_RenderFillRect(pWait->renderer, &panel);
        SDL_SetRenderDrawColor(pWait->renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(pWait->renderer, &panel);

        int panel_center_x = panel.x + (panel.w / 2);

        // Rita statiska texter (text_draw centrerar på x och y)
        text_draw(pWait->titleText,    panel_center_x, panel.y + 42 + text_get_height(pWait->titleText) / 2);
        text_draw(pWait->subtitleText, panel_center_x, panel.y + 125 + text_get_height(pWait->subtitleText) / 2);
        text_draw(pWait->enterText,    panel_center_x, panel.y + 180 + text_get_height(pWait->enterText) / 2);

        // Inputruta
        SDL_Rect inputBox;
        inputBox.x = panel.x + 70;
        inputBox.y = panel.y + 255;
        inputBox.w = panel.w - 140;
        inputBox.h = 78;

        SDL_SetRenderDrawColor(pWait->renderer, 24, 34, 46, 255);
        SDL_RenderFillRect(pWait->renderer, &inputBox);
        SDL_SetRenderDrawColor(pWait->renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(pWait->renderer, &inputBox);

        // Rita input-texten vänsterställd (text_draw_at = övre vänstra hörnet)
        if (strlen(buffer) > 0)
            text_draw_at(pWait->inputText, inputBox.x + 24, inputBox.y + 12);

        // Esc-text
        text_draw(pWait->escText, panel_center_x, panel.y + 355 + text_get_height(pWait->escText) / 2);

        // Felmeddelande
        if (error_message && error_message[0] != '\0')
        {
            text_set(pWait->errorText, error_message, red);
            text_draw(pWait->errorText, panel_center_x, panel.y + 392 + text_get_height(pWait->errorText) / 2);
        }

        SDL_RenderPresent(pWait->renderer);
    }

    SDL_StopTextInput();
    return 1;
}

void renderWaitingScreen(waitForPlayers *pWait, gameState *state)
{
    SDL_Color white = {255, 255, 255, 255};
    SDL_RenderCopy(pWait->renderer, pWait->background, NULL, NULL);

    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(pWait->renderer, &windowWidth, &windowHeight);

    // Uppdatera connected-text
    int connectedPlayers = countActivePlayers(state);
    char text[64];
    snprintf(text, sizeof(text), "%d/%d CONNECTED", connectedPlayers, MAX_PLAYERS);
    text_set(pWait->connectedText, text, white);
    text_draw(pWait->connectedText, windowWidth / 2, windowHeight / 6 + text_get_height(pWait->connectedText) / 2);

    if (connectedPlayers >= 1)
    {
        text_draw(pWait->startText, windowWidth / 2, windowHeight - 180 + text_get_height(pWait->startText) / 2);
    }

    SDL_RenderPresent(pWait->renderer);
}