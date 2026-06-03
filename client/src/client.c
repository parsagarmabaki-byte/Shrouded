#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "lobby.h"
#include "main_menu.h"
#include "game.h"
#include "SFX.h"
#include "client_network.h"
#include "emergency_meeting.h"

int main()
{
    waitForPlayers lobby = {0};
    char server_ip[64] = "127.0.0.1";

    if (!initiate(&lobby)) return 1;

    bool app_running = true;
    while (app_running)
    {
        Client client = {0};
        SDL_Event event;
        gameState state = {0};
        AudioAssets audio = {0};
        bool running = true;
        bool return_to_menu = false;
        char connection_error[128] = "";

        SDL_RenderSetLogicalSize(lobby.renderer, 0, 0);

        // Visa huvudmenyn först. Om användaren valde Exit (eller stängde fönstret)
        // städar vi upp och avslutar utan att gå vidare till IP-prompten.
        if (!showMainMenu(&lobby))
        {
            break;
        }

        while (running)
        {
            if (!promptServerAddress(&lobby, server_ip, sizeof(server_ip), connection_error))
            {
                running = false;
                app_running = false;
                break;
            }

            if (init_client(&client, server_ip, connection_error, sizeof(connection_error)))
            {
                break;
            }
        }

        if (!running)
        {
            clean_client(&client);
            continue;
        }

        if (!send_join(&client))
        {
            clean_client(&client);
            cleanLobby(&lobby);
            return 1;
        }
        if (!init_audio())
        {
            clean_client(&client);
            cleanLobby(&lobby);
            return 1;
        }
        if (!load_audio(&audio))
        {
            cleanup_audio(&audio);
            clean_client(&client);
            cleanLobby(&lobby);
            return 1;
        }

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
                    app_running = false;
                }
                if (event.type == SDL_KEYDOWN)
                {
                    if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                    {
                        send_leave_message(&client);
                        running = false;
                        app_running = false;
                    }
                    else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
                    {
                        if (state.local_player_id == state.host_player_id && countActivePlayers(&state) >= 2 && state.phase == GAME_LOBBY)
                            send_start_game(&client);
                    }
                }
            }

            collect_packets(&client, &state, NULL, &audio);

            if (state.phase != GAME_LOBBY)
                running = false;
            else
                renderWaitingScreen(&lobby, &state);
        }

        // Game-loop
        if (state.phase != GAME_LOBBY)
        {
            stop_music();
            return_to_menu = runGame(&client, &lobby, &state, &audio);
        }

        cleanup_audio(&audio);
        clean_client(&client);

        if (!return_to_menu)
            app_running = false;
    }

    cleanLobby(&lobby);

    return 0;
}
