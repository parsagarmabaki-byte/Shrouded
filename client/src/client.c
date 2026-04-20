#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <stdbool.h>
#include <stdio.h>
#include "network.h"
#include "lobby.h"
#include "game.h"

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

        if (state.phase != GAME_LOBBY)
            running = false;
        else
            renderWaitingScreen(&lobby, &state);
    }

    // Game-loop
    if (state.phase != GAME_LOBBY)
        runGame(&client, &lobby, &state);

    cleanLobby(&lobby);
    cleanClient(&client);

    return 0;
}
