#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
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
    SDL_RenderPresent(pWait->renderer);

    SDL_DestroyTexture(texture);

}


int main()
{
    Client client = {0};
    waitForPlayers lobby = {0};
    SDL_Event event;
    gameState state = {0};
    int running = 1;


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
    if (!sendMessage(&client))
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
                running = 0;
            }
        }
        receive_game_state(client.socket, client.recievepacket, &state);
        renderWaitingScreen(&lobby, &state);
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