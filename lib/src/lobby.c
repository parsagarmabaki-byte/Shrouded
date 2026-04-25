#include "lobby.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static SDL_Texture *create_text_texture(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                                        SDL_Color color, int *width, int *height)
{
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return NULL;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture)
    {
        SDL_FreeSurface(surface);
        return NULL;
    }

    if (width) *width = surface->w;
    if (height) *height = surface->h;
    SDL_FreeSurface(surface);
    return texture;
}

static void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color, SDL_Rect dst)
{
    SDL_Texture *texture = create_text_texture(renderer, font, text, color, &dst.w, &dst.h);
    if (!texture) return;

    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
}

static void draw_centered_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color,
                               int center_x, int y)
{
    int width = 0;
    int height = 0;
    SDL_Texture *texture = create_text_texture(renderer, font, text, color, &width, &height);
    if (!texture) return;

    SDL_Rect dst = {
        .x = center_x - (width / 2),
        .y = y,
        .w = width,
        .h = height
    };

    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
}

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

int promptServerAddress(waitForPlayers *pWait, char *buffer, size_t buffer_size, const char *error_message)
{
    SDL_Event event;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color muted = {210, 210, 210, 255};
    SDL_Color red = {255, 120, 120, 255};
    int windowWidth;
    int windowHeight;
    int panel_center_x;
    int finished = 0;

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

        SDL_RenderCopy(pWait->renderer, pWait->background, NULL, NULL);

        SDL_GetRendererOutputSize(pWait->renderer, &windowWidth, &windowHeight);

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

        panel_center_x = panel.x + (panel.w / 2);
        draw_centered_text(pWait->renderer, pWait->Font, "CONNECT TO SERVER", white,
                           panel_center_x, panel.y + 42);
        draw_centered_text(pWait->renderer, pWait->Font, "Enter the server IP address below", muted,
                           panel_center_x, panel.y + 125);
        draw_centered_text(pWait->renderer, pWait->Font, "Press Enter to connect", muted,
                           panel_center_x, panel.y + 180);

        SDL_Rect inputBox;
        inputBox.x = panel.x + 70;
        inputBox.y = panel.y + 255;
        inputBox.w = panel.w - 140;
        inputBox.h = 78;

        SDL_SetRenderDrawColor(pWait->renderer, 24, 34, 46, 255);
        SDL_RenderFillRect(pWait->renderer, &inputBox);
        SDL_SetRenderDrawColor(pWait->renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(pWait->renderer, &inputBox);

        SDL_Rect textRect;
        textRect.x = inputBox.x + 24;
        textRect.y = inputBox.y + 12;
        textRect.w = 0;
        textRect.h = 0;
        draw_text(pWait->renderer, pWait->Font, buffer, white, textRect);

        draw_centered_text(pWait->renderer, pWait->Font, "Esc closes the client", muted,
                           panel_center_x, panel.y + 355);

        if (error_message && error_message[0] != '\0')
        {
            draw_centered_text(pWait->renderer, pWait->Font, error_message, red,
                               panel_center_x, panel.y + 392);
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

    int connectedPlayers = countActivePlayers(state);
    char text[64];
    snprintf(text, sizeof(text), "%d/%d CONNECTED", connectedPlayers, MAX_PLAYERS);

    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(pWait->renderer, &windowWidth, &windowHeight);
    int text_w = 0;
    int text_h = 0;
    SDL_Texture *texture = create_text_texture(pWait->renderer, pWait->Font, text, white, &text_w, &text_h);
    if (!texture) return;

    SDL_Rect dst = {
        .x = (windowWidth - text_w) / 2,
        .y = windowHeight / 6,
        .w = text_w,
        .h = text_h
    };

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
