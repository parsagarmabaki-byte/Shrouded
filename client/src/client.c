#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdio.h>
#include "network.h"

typedef struct
{
    UDPsocket socket;
    IPaddress serverAddr;
    UDPpacket *sendpacket;
} Client;

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *Font;
} waitForPlayers;

int initNetworking()
{
    if (SDLNet_Init() < 0)
    {
        printf("SDLNet_Init failed: %s\n", SDLNet_GetError());
        return 0;
    }
    return 1;
}
int openClientSocket(Client *client)
{
    client->socket = SDLNet_UDP_Open(0);
    if (!client->socket)
    {
        printf("Failed to open client socket: %s\n", SDLNet_GetError());
        SDLNet_Quit();
        return 0;
    }
    return 1;
}
int resolveServerAdress(Client *client, const char *host)
{
    if (SDLNet_ResolveHost(&client->serverAddr, host, SERVER_PORT) < 0)
    {
        printf("Failed to resolve host: %s\n", SDLNet_GetError());
        return 0;
    }
    return 1;
}
int allocateSendPacket(Client *client, int size)
{
    client->sendpacket = SDLNet_AllocPacket(size);
    if (!client->sendpacket)
    {
        printf("Failed to allocate send packet: %s\n", SDLNet_GetError());
        return 0;
    }
    return 1;
}
int sendMessage(Client *client)
{
    joinMessage join;
    join.type = MSG_JOIN;

    memcpy(client->sendpacket->data, &join, sizeof(joinMessage));
    client->sendpacket->len = sizeof(joinMessage);
    client->sendpacket->address = client->serverAddr;

    if (SDLNet_UDP_Send(client->socket, -1, client->sendpacket) == 0)
    {
        printf("Failed to send join packet: %s\n", SDLNet_GetError());
        return 0;
    }
    else
    {
        printf("Join packet sent to server\n");
        return 1;
    }
}
void cleanClient(Client *client)
{
    if (client->sendpacket)
    {
        SDLNet_FreePacket(client->sendpacket);
        client->sendpacket = NULL;
    }
    if (client->socket)
    {
        SDLNet_UDP_Close(client->socket);
        client->socket = NULL;
    }
    SDLNet_Quit();
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

    pWait->window = SDL_CreateWindow(
        "Shrouded Lobby",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600, 0);

    if (!pWait->window)
    {
        printf("SDL_CreateWindow: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    pWait->renderer = SDL_CreateRenderer(pWait->window, -1, SDL_RENDERER_ACCELERATED);
    if (!pWait->renderer)
    {
        printf("SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(pWait->window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    pWait->Font = TTF_OpenFont("assets/fonts/Roboto-Regular.ttf", 32);
    if (!pWait->Font)
    {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_DestroyRenderer(pWait->renderer);
        SDL_DestroyWindow(pWait->window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    return 1;
}

void renderWaitingScreen(waitForPlayers *pWait)
{
    SDL_Color white = {255, 255, 255, 255};

    SDL_Surface *surface = TTF_RenderText_Blended(
        pWait->Font,
        "Waiting for players...",
        white);
    if (!surface)
    {
        printf("TTF_RenderText_Blended: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(pWait->renderer, surface);
    if (!texture)
    {
        printf("SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dst;
    dst.w = surface->w;
    dst.h = surface->h;
    dst.x = (800 - dst.w) / 2;
    dst.y = (600 - dst.h) / 2;

    SDL_FreeSurface(surface);

    SDL_SetRenderDrawColor(pWait->renderer, 0, 0, 0, 255);
    SDL_RenderClear(pWait->renderer);

    SDL_RenderCopy(pWait->renderer, texture, NULL, &dst);
    SDL_RenderPresent(pWait->renderer);

    SDL_DestroyTexture(texture);
}
int main()
{
    Client client = {0};

    if (!initNetworking())
    {
        return 1;
    }
    if (!openClientSocket(&client))
    {
        return 1;
    }
    if (!resolveServerAdress(&client, "127.0.0.1"))
    {
        return 1;
    }
    if (!allocateSendPacket(&client, 512))
    {
        return 1;
    }
    if (!sendMessage(&client))
    {
        return 1;
    }

    waitForPlayers lobby = {0};
    SDL_Event event;
    int running = 1;

    if (!initiate(&lobby))
    {
        return 1;
    }
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
            }
        }
        renderWaitingScreen(&lobby);
    }
    TTF_CloseFont(lobby.Font);
    SDL_DestroyRenderer(lobby.renderer);
    SDL_DestroyWindow(lobby.window);
    TTF_Quit();
    SDL_Quit();

    cleanClient(&client);

    return 0;
}