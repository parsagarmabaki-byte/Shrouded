#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "network.h"
#include <SDL2/SDL_image.h>
typedef struct
{
    UDPsocket socket;
    IPaddress serverAddr;
    UDPpacket *sendpacket;
    UDPpacket *recievepacket;
} Client;

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *Font;
    SDL_Texture *background;
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
int allocateReceivePacket(Client *client, int size)
{
    client->recievepacket = SDLNet_AllocPacket(size);
    if (!client->recievepacket)
    {
        printf("Failed to allocate receive packet: %s\n", SDLNet_GetError());
        return 0;
    }
    return 1;
}
int sendJoinMessage(Client *client)
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
int sendStartMessage(Client *client)
{
    startGameMessage start;
    start.type = MSG_START_GAME;

    memcpy(client->sendpacket->data, &start, sizeof(startGameMessage));
    client->sendpacket->len = sizeof(startGameMessage);
    client->sendpacket->address = client->serverAddr;
    if (SDLNet_UDP_Send(client->socket, -1, client->sendpacket) == 0)
    {
        printf("Failed to send start packet: %s\n", SDLNet_GetError());
        return 0;
    }
    else
    {
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
    if (client->recievepacket)
    {
        SDLNet_FreePacket(client->recievepacket);
        client->recievepacket = NULL;
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

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)){
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
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    pWait->renderer = SDL_CreateRenderer(pWait->window, -1, SDL_RENDERER_ACCELERATED);
    if (!pWait->renderer)
    {
        printf("SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(pWait->window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    pWait->Font = TTF_OpenFont("assets/fonts/BebasNeue-Regular.ttf", 60);
    if (!pWait->Font)
    {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_DestroyRenderer(pWait->renderer);
        SDL_DestroyWindow(pWait->window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    SDL_Surface *bgSurface = IMG_Load("assets/lobbyscreen/waitingforplayers.png");
    if (!bgSurface) {
        printf("IMG_Load: %s\n", IMG_GetError());
        TTF_CloseFont(pWait->Font);
        SDL_DestroyRenderer(pWait->renderer);
        SDL_DestroyWindow(pWait->window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    pWait->background = SDL_CreateTextureFromSurface(pWait->renderer, bgSurface);
    SDL_FreeSurface(bgSurface);

    if (!pWait->background){
        printf("SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
        TTF_CloseFont(pWait->Font);
        SDL_DestroyRenderer(pWait->renderer);
        SDL_DestroyWindow(pWait->window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    return 1;

}
int countActivePlayers(gameState *state)
{
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (state->players[i].active)
        {
            count++;
        }
    }
    return count;
}

void renderWaitingScreen(waitForPlayers *pWait, gameState *state)
{
    SDL_Color white = {255, 255, 255, 255};

    SDL_RenderCopy(pWait->renderer, pWait->background, NULL, NULL);

    int connectedPlayers = countActivePlayers(state);

    char text[64];
    snprintf(text,sizeof(text), "%d/%d CONNECTED", connectedPlayers, MAX_PLAYERS);

    SDL_Surface *surface = TTF_RenderText_Blended(
        pWait->Font,
        text,
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

    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(pWait->renderer, &windowWidth, &windowHeight);

    dst.x = (windowWidth - dst.w) / 2;
    dst.y = windowHeight / 6;

    SDL_FreeSurface(surface);

    SDL_RenderCopy(pWait->renderer, texture, NULL, &dst);

    SDL_DestroyTexture(texture);

    if (connectedPlayers >= 1)
    {
        SDL_Surface *startSurface = TTF_RenderText_Blended(
        pWait->Font,
        "PRESS SPACE TO START",
        white
    );
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
int main()
{
    Client client = {0};
    waitForPlayers lobby = {0};
    SDL_Event event;
    gameState state = {0};
    bool running = true;


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
    if (!allocateReceivePacket(&client, 512))
    {
        return 1;
    }
    if (!sendJoinMessage(&client))
    {
        return 1;
    }
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
                send_leave(client.socket, client.serverAddr);
                running = false;
            }
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    send_leave(client.socket, client.serverAddr);
                    running = false;
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
                {
                    if (countActivePlayers(&state) >= 1 && state.phase == GAME_LOBBY)
                    {
                        sendStartMessage(&client);
                    }
                }
                
            }
            
        }
        receive_game_state(client.socket, client.recievepacket, &state);
        if (state.phase == GAME_RUNNING)
        {
            printf("Game is starting...\n");
            running = false;
        } else
        {
            renderWaitingScreen(&lobby, &state);
        }

    }
    TTF_CloseFont(lobby.Font);
    SDL_DestroyTexture(lobby.background);
    SDL_DestroyRenderer(lobby.renderer);
    SDL_DestroyWindow(lobby.window);
    TTF_Quit();
    SDL_Quit();

    cleanClient(&client);

    return 0;
}