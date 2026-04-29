#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "lobby.h"
#include "game.h"
#include "SFX.h"
#include "client_network.h"
#include "emergency_meeting.h"

int main()
{
    Client client = {0};
    waitForPlayers lobby = {0};
    SDL_Event event;
    gameState state = {0};
    AudioAssets audio;
    bool running = true;
    char server_ip[64] = "127.0.0.1";
    char connection_error[128] = "";

    if (!initiate(&lobby)) return 1;
    while (running)
    {
        if (!promptServerAddress(&lobby, server_ip, sizeof(server_ip), connection_error))
        {
            cleanLobby(&lobby);
            return 0;
        }

        if (init_client(&client, server_ip, connection_error, sizeof(connection_error)))
        {
            break;
        }
    }

    if (!send_join(&client))
    {
        cleanLobby(&lobby);
        clean_client(&client);
        return 1;
    }
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
                send_leave_message(&client);
                running = false;
            }
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    send_leave_message(&client);
                    running = false;
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
                {
                    if (countActivePlayers(&state) >= 1 && state.phase == GAME_LOBBY)
                        send_start_game(&client);
                }
            }
        }
        
        collect_packets(&client, &state, NULL);

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
