#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "network.h"
#include "network_data.h"
#include "game_map.h"
#include "player_movement.h"

typedef struct
{
    UDPsocket socket;
    IPaddress serverAddr;
    UDPpacket *recievepacket;
} Client;

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *Font;
    SDL_Texture *background;
} waitForPlayers;

int allocatePacket(UDPpacket **packet, int size)
{
    *packet = SDLNet_AllocPacket(size);
    if (!*packet)
    {
        printf("Failed to allocate packet: %s\n", SDLNet_GetError());
        return 0;
    }
    return 1;
}

void cleanClient(Client *client)
{
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

void sendInput(Client *client, gameState *state)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    clientInput input = {0};
    input.type = MSG_CLIENT_INPUT;
    input.player_id = state->local_player_id;
    input.up    = keys[SDL_SCANCODE_W];
    input.down  = keys[SDL_SCANCODE_S];
    input.left  = keys[SDL_SCANCODE_A];
    input.right = keys[SDL_SCANCODE_D];
    send_client_input(client->socket, client->serverAddr, &input);
    printf("sending: up=%d down=%d left=%d right=%d\n", input.up, input.down, input.left, input.right);
}

void runGame(Client *client, waitForPlayers *lobby, gameState *state)
{
    SDL_Renderer *renderer = lobby->renderer;

    int window_width, window_height;
    SDL_GetWindowSize(lobby->window, &window_width, &window_height);
    SDL_RenderSetLogicalSize(renderer, window_width, window_height);

    SDL_Texture *mapTexture = loading_img(renderer, "assets/images/Game_map.png");
    if (!mapTexture) { printf("Failed to load map\n"); return; }

    SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "assets/sprites/charspritesv2.png");
    if (!playerTexture) { printf("Failed to load player sprite\n"); return; }

    int local_id = state->local_player_id;
    Player player = init_player(window_width, window_height);
    player.Hitbox.x = state->players[local_id].x;
    player.Hitbox.y = state->players[local_id].y;

    Camera cam = {0, 0, window_width, window_height};

    SDL_RaiseWindow(lobby->window);
    SDL_SetWindowInputFocus(lobby->window);

    SDL_Event event;
    bool running = true;
    Uint64 last = SDL_GetPerformanceCounter();

    while (running)
    {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency();
        last = now;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            {
                send_leave(client->socket, client->serverAddr);
                running = false;
            }
        }

        // Skicka input till servern
        sendInput(client, state);

        // Ta emot uppdaterad gameState från servern
        receive_game_state(client->socket, client->recievepacket, state);

        // Uppdatera lokal spelarposition från servern
        player.Hitbox.x = state->players[local_id].x;
        player.Hitbox.y = state->players[local_id].y;

        // Riktning baserat på input
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_W]) player.direction = DIR_UP;
        else if (keys[SDL_SCANCODE_S]) player.direction = DIR_DOWN;
        else if (keys[SDL_SCANCODE_A]) player.direction = DIR_LEFT;
        else if (keys[SDL_SCANCODE_D]) player.direction = DIR_RIGHT;

        // Animering baserat på input
        bool moving = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_S] ||
                      keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_D];
        if (moving)
        {
            player.animation_timer += dt;
            if (player.animation_timer > 0.1f)
            {
                player.current_frame = (player.current_frame + 1) % 4;
                player.animation_timer = 0;
            }
        }
        else
        {
            player.current_frame = 2;
        }

        // Kamera följer lokal spelare
        camera_follow(&cam, player.Hitbox.x, player.Hitbox.y, player.Hitbox.w, player.Hitbox.h);

        // Rita karta
        render_map(renderer, mapTexture, &cam);

        // Rita alla aktiva spelare
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (state->players[i].active)
            {
                Player p = player;
                p.Hitbox.x = state->players[i].x;
                p.Hitbox.y = state->players[i].y;

                if (i != local_id)
                {
                    // Andra spelare — idle animation
                    p.current_frame = 2;
                    p.direction = DIR_DOWN;
                }

                renderPlayer(renderer, &p, playerTexture, &cam);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    SDL_DestroyTexture(mapTexture);
    SDL_DestroyTexture(playerTexture);
}

int main()
{
    Client client = {0};
    waitForPlayers lobby = {0};
    SDL_Event event;
    gameState state = {0};
    bool running = true;

    if (!init_client(&client.socket, &client.serverAddr)) return 1;
    if (!allocatePacket(&client.recievepacket, 512)) return 1;
    if (!send_join(client.socket, client.serverAddr)) return 1;
    if (!initiate(&lobby)) return 1;

    // Lobby-loop
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
                        send_start_game(client.socket, client.serverAddr);
                }
            }
        }

        receive_game_state(client.socket, client.recievepacket, &state);

        if (state.phase == GAME_RUNNING)
        {
            running = false;
        }
        else
        {
            renderWaitingScreen(&lobby, &state);
        }
    }

    // Game-loop
    if (state.phase == GAME_RUNNING)
    {
        runGame(&client, &lobby, &state);
    }

    // Städa upp
    TTF_CloseFont(lobby.Font);
    SDL_DestroyTexture(lobby.background);
    SDL_DestroyRenderer(lobby.renderer);
    SDL_DestroyWindow(lobby.window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    cleanClient(&client);

    return 0;
}