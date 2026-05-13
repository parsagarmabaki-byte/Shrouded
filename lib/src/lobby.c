#include "lobby.h"
#include "text.h"
#include "ip_config.h"
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

    SDL_Surface *enterIpSurface = IMG_Load("assets/lobbyscreen/ENTERIP.png");
    if (!enterIpSurface)
    {
        printf("IMG_Load ENTERIP.png: %s\n", IMG_GetError());
        cleanLobby(pWait);
        return 0;
    }

    pWait->enterIpBackground = SDL_CreateTextureFromSurface(pWait->renderer, enterIpSurface);
    SDL_FreeSurface(enterIpSurface);

    if (!pWait->enterIpBackground)
    {
        printf("SDL_CreateTextureFromSurface ENTERIP.png: %s\n", SDL_GetError());
        cleanLobby(pWait);
        return 0;
    }

    // Skapa Text-objekt med storlekar som passar respektive vy.
    const char *fontPath = "assets/fonts/BebasNeue-Regular.ttf";

    pWait->titleText     = text_create(pWait->renderer, fontPath, 64);
    pWait->subtitleText  = text_create(pWait->renderer, fontPath, 30);
    pWait->enterText     = text_create(pWait->renderer, fontPath, 30);
    pWait->escText       = text_create(pWait->renderer, fontPath, 28);
    pWait->errorText     = text_create(pWait->renderer, fontPath, 30);
    pWait->inputText     = text_create(pWait->renderer, fontPath, 48);
    pWait->connectedText = text_create(pWait->renderer, fontPath, 60);
    pWait->startText     = text_create(pWait->renderer, fontPath, 60);
    pWait->ipButtonText  = text_create(pWait->renderer, fontPath, 15);

    if (!pWait->titleText || !pWait->subtitleText || !pWait->enterText ||
        !pWait->escText || !pWait->errorText || !pWait->inputText ||
        !pWait->connectedText || !pWait->startText || !pWait->ipButtonText)
    {
        printf("text_create failed for one or more Text objects\n");
        cleanLobby(pWait);
        return 0;
    }

    // Sätt statiska texter en gång — dessa ändras aldrig
    SDL_Color white = {245, 248, 255, 255};
    SDL_Color muted = {165, 175, 190, 255};

    text_set(pWait->titleText,    "SERVER CONNECTION",        white);
    text_set(pWait->subtitleText, "TYPE SERVER IP ADDRESS",    muted);
    text_set(pWait->enterText,    "ENTER TO CONNECT",          muted);
    text_set(pWait->escText,      "ESC TO RETURN",             muted);
    text_set(pWait->startText,    "PRESS SPACE TO START",              white);
    text_set(pWait->ipButtonText, "FETCH MY IP - MAC/LINUX ONLY",      white);

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
    text_destroy(pWait->ipButtonText);

    if (pWait->enterIpBackground) SDL_DestroyTexture(pWait->enterIpBackground);
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
    SDL_Color white = {245, 248, 255, 255};
    SDL_Color muted = {105, 118, 135, 255};
    SDL_Color red   = {255, 125, 125, 255};
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

            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                SDL_Rect enter_ip_rect = get_enter_ip_rect(pWait->renderer, pWait->enterIpBackground);
                SDL_Rect input_box = {
                    enter_ip_rect.x + (int)(enter_ip_rect.w * 0.18f),
                    enter_ip_rect.y + (int)(enter_ip_rect.h * 0.47f),
                    (int)(enter_ip_rect.w * 0.54f),
                    (int)(enter_ip_rect.h * 0.19f)
                };
                SDL_Rect my_ip_button = get_my_ip_button_rect(input_box);

                if (point_in_rect(event.button.x, event.button.y, my_ip_button))
                {
                    get_local_ip_address(buffer, buffer_size);
                    textChanged = 1;
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
            else
                text_set(pWait->inputText, "127.0.0.1", muted);
            textChanged = 0;
        }

        // Uppdatera felmeddelande (sätts en gång per anrop till promptServerAddress)
        // Görs utanför loopen om det vore statiskt, men error_message kan vara NULL
        // så vi sätter det här en gång (behöver egentligen bara göras en gång,
        // men text_set är billig jämfört med att rita varje frame)

        SDL_GetRendererOutputSize(pWait->renderer, &windowWidth, &windowHeight);

        SDL_Rect enter_ip_rect = get_enter_ip_rect(pWait->renderer, pWait->enterIpBackground);
        render_connection_background(pWait->renderer, pWait->enterIpBackground, pWait->background, enter_ip_rect);

        int screen_center_x = enter_ip_rect.x + enter_ip_rect.w / 2;

        text_draw(pWait->titleText, screen_center_x, enter_ip_rect.y + (int)(enter_ip_rect.h * 0.20f));
        text_draw(pWait->subtitleText, screen_center_x, enter_ip_rect.y + (int)(enter_ip_rect.h * 0.30f));
        text_draw(pWait->enterText, screen_center_x, enter_ip_rect.y + (int)(enter_ip_rect.h * 0.75f));

        // Inputruta
        SDL_Rect inputBox;
        inputBox.x = enter_ip_rect.x + (int)(enter_ip_rect.w * 0.18f);
        inputBox.y = enter_ip_rect.y + (int)(enter_ip_rect.h * 0.47f);
        inputBox.w = (int)(enter_ip_rect.w * 0.54f);
        inputBox.h = (int)(enter_ip_rect.h * 0.19f);

        // Rita input-texten vänsterställd (text_draw_at = övre vänstra hörnet)
        SDL_Rect my_ip_button = get_my_ip_button_rect(inputBox);
        int input_text_x = inputBox.x + (int)(inputBox.w * 0.035f);
        int input_text_y = inputBox.y + (inputBox.h - text_get_height(pWait->inputText)) / 2;
        text_draw_at(pWait->inputText, input_text_x, input_text_y);

        if ((SDL_GetTicks() / 500) % 2 == 0)
        {
            int cursor_x = input_text_x + text_get_width(pWait->inputText) + 6;
            int text_height = text_get_height(pWait->inputText);
            int cursor_y = input_text_y;
            if (strlen(buffer) == 0)
                cursor_x = input_text_x;

            SDL_SetRenderDrawColor(pWait->renderer, 245, 226, 176, 255);
            SDL_RenderDrawLine(pWait->renderer, cursor_x, cursor_y, cursor_x, cursor_y + text_height);
        }

        // Esc-text
        text_draw(pWait->escText, screen_center_x, enter_ip_rect.y + (int)(enter_ip_rect.h * 0.81f));

        render_my_ip_button(pWait->renderer, pWait->ipButtonText, my_ip_button);

        // Felmeddelande
        if (error_message && error_message[0] != '\0')
        {
            text_set(pWait->errorText, error_message, red);
            text_draw(pWait->errorText, screen_center_x, enter_ip_rect.y + (int)(enter_ip_rect.h * 0.87f));
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
