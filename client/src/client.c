#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <stdbool.h>
#include <stdio.h>
#include "network.h"
#include "lobby.h"
#include "game.h"
#include "SFX.h"
#include "client_network.h"

static int init_client(UDPsocket *socket, IPaddress *server_addr)
{
    if (!init_network_socket(socket, 0))
    {
        return 0;
    }

    if (SDLNet_ResolveHost(server_addr, "127.0.0.1", SERVER_PORT) != 0)
    {
        printf("SDLNet_ResolveHost error: %s\n", SDLNet_GetError());
        return 0;
    }

    return 1;
}

static int send_client_message(UDPsocket socket, IPaddress server_addr, MessageType type)
{
    UDPpacket *packet = create_packet(512);
    if (!packet)
    {
        return 0;
    }

    if (!send_packet_data(socket, packet, server_addr, &type, sizeof(type)))
    {
        SDLNet_FreePacket(packet);
        return 0;
    }

    SDLNet_FreePacket(packet);
    return 1;
}

static int send_join(UDPsocket socket, IPaddress server_addr)
{
    joinMessage join = {0};
    join.type = MSG_JOIN;
    return send_client_message(socket, server_addr, join.type);
}

static int send_start_game(UDPsocket socket, IPaddress server_addr)
{
    startGameMessage start = {0};
    start.type = MSG_START_GAME;
    return send_client_message(socket, server_addr, start.type);
}

static void clean_client(Client *client)
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
    AudioAssets audio;
    bool running = true;

    if (!init_client(&client.socket, &client.serverAddr)) return 1;
    client.recievepacket = create_packet(512);
    if (!client.recievepacket) return 1;
    if (!send_join(client.socket, client.serverAddr)) return 1;
    if (!initiate(&lobby)) return 1;
    if (!init_audio()) return 1;
    if (!load_audio(&audio)){cleanup_audio(&audio);return 1;}

    play_lobby_music(audio.lobby_music, -1);

    // Lobby-loop
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                send_leave_message(client.socket, client.serverAddr);
                running = false;
            }
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    send_leave_message(client.socket, client.serverAddr);
                    running = false;
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
                {
                    if (countActivePlayers(&state) >= 1 && state.phase == GAME_LOBBY)
                        send_start_game(client.socket, client.serverAddr);
                }
            }
        }
        
        collect_packets(&client, &state);

        if (state.phase != GAME_LOBBY)
            running = false;
        else
            renderWaitingScreen(&lobby, &state);
    }

    // Game-loop
    if (state.phase != GAME_LOBBY)
    {
        stop_music();
        runGame(&client, &lobby, &state);
    }

    cleanup_audio(&audio);
    cleanLobby(&lobby);
    clean_client(&client);

    return 0;
}
