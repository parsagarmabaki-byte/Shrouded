#define SDL_MAIN_HANDLED
#include <SDL2/SDL_net.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "network.h"
#include "network_data.h"
#include "player_movement.h"
#include "imposter_ability.h"
#define PACKET_SIZE 512

static int init_server(UDPsocket *socket)
{
    return init_network_socket(socket, SERVER_PORT);
}

static int send_game_state(UDPsocket socket, UDPpacket *packet, IPaddress addr, gameState *state)
{
    return send_packet_data(socket, packet, addr, state, sizeof(*state));
}

static int send_kill_msg(UDPsocket socket, UDPpacket *packet, IPaddress address, KillEventMsg *msg)
{
    return send_packet_data(socket, packet, address, msg, sizeof(*msg));
}

void cleanupServer(UDPsocket server_socket, UDPpacket *receive_packet, UDPpacket *send_packet)
{
    if (receive_packet)
    {
        SDLNet_FreePacket(receive_packet);
    }
    if (send_packet)
    {
        SDLNet_FreePacket(send_packet);
    }
    if (server_socket)
    {
        SDLNet_UDP_Close(server_socket);
    }
    SDLNet_Quit();
}

int get_player_id_from_sender(IPaddress *clientAddresses, int *clientUsed, IPaddress sender)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i] &&
            clientAddresses[i].host == sender.host &&
            clientAddresses[i].port == sender.port)
        {
            return i;
        }
    }
    return -1;
}

int addToLobby(gameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr)
{
    float spawnX[MAX_PLAYERS] = {1290, 1150, 1420, 1000, 1290, 1150};
    float spawnY[MAX_PLAYERS] = {665, 665, 850, 850, 1000, 1000};

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!clientUsed[i])
        {
            clientUsed[i] = 1;
            clientAddresses[i] = addr;

            state->players[i].active = 1;
            state->players[i].isAlive = 1;
            state->players[i].player_id = i;
            state->players[i].x = spawnX[i];
            state->players[i].y = spawnY[i];
            state->players[i].current_frame = 2;
            state->players[i].direction = DIR_DOWN;
            state->players[i].isImpostor = 0;
            state->players[i].kill_cooldown_start = 0;
            state->players[i].kill_cooldown_active = false;
            state->players[i].emergency_meeting = 1;
            state->emergency_meeting_reported_id = -1;
            return i;
        }
    }
    return -1;
}

void spawn_players(gameState *state)
{
    float spawnX[MAX_PLAYERS] = {1290, 1150, 1420, 1000, 1290, 1150};
    float spawnY[MAX_PLAYERS] = {665, 665, 850, 850, 1000, 1000};

    for (int i = 0; i < MAX_PLAYERS; i++)
    {

        state->players[i].x = spawnX[i];
        state->players[i].y = spawnY[i];
        state->players[i].current_frame = 2;
        state->players[i].direction = DIR_DOWN;
        state->players[i].kill_cooldown_start = 0;
        state->players[i].kill_cooldown_active = false;
    }
}

int countActivePlayers(gameState *state)
{
    int active_players = 0;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (state->players[i].active)
            active_players++;
    }

    return active_players;
}

int designateImpostor(gameState *state)
{
    int active_player_count = countActivePlayers(state);
    int chosen_active_player = 0;
    int active_player_index = 0;

    chosen_active_player = rand() % active_player_count;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!state->players[i].active)
            continue;

        if (active_player_index == chosen_active_player)
        {
            state->players[i].isImpostor = 1;
            return chosen_active_player;
        }

        active_player_index++;
    }
    return -1; // alla aktiva spelare gicks igenom utan match (borde inte hända)
}

static void shuffle_tasks(TaskType *arr, int n)
{
    for (int i = n - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        TaskType tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

void check_win_condition(gameState *state)
{
    int alive_impostor = 0;
    int alive_crewmates = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (state->players[i].isAlive)
        {
            alive_crewmates++;
        }

        if (state->players[i].isImpostor)
        {
            alive_impostor++;
        }

    } // Kolla om impostorn finns
    if (!alive_impostor)
    {
        state->phase = GAME_CREWMATES_WIN;
    }
    else if (alive_impostor >= alive_crewmates)
    {
        state->phase = GAME_IMPOSTOR_WIN;
    }

    // Lägg till task win-conditions
}

void broadcastGameState(UDPsocket socket, UDPpacket *packet, gameState *state, IPaddress *clientAddresses, int *clientUsed)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i])
        {
            state->local_player_id = i;
            if (!send_game_state(socket, packet, clientAddresses[i], state))
            {
                printf("Failed to send game state to player %d\n", i);
            }
        }
    }
}

void broadcast_Kill_msg(UDPsocket socket, UDPpacket *packet, KillEventMsg *msg, IPaddress *clientAddresses, int *clientUsed)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i])
        {
            if (!send_kill_msg(socket, packet, clientAddresses[i], msg))
            {
                printf("Failed to send game state to player %d\n", i);
            }
        }
    }
}

void broadcast_emergency_meeting_msg(UDPsocket socket, UDPpacket *packet, KillEventMsg *msg, IPaddress *clientAddresses, int *clientUsed)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i])
        {
            if (!send_kill_msg(socket, packet, clientAddresses[i], msg))
            {
                printf("Failed to send game state to player %d\n", i);
            }
        }
    }
}

int removeFromLobby(gameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr)
{
    int player = get_player_id_from_sender(clientAddresses, clientUsed, addr);
    if (player >= 0)
    {
        clientUsed[player] = 0;
        state->players[player].active = 0;
        state->players[player].player_id = -1;
        return player;
    }
    return -1;
}

void handleInput(gameState *state, clientInput *input, float dt)
{
    int id = input->player_id;
    if (id < 0 || id >= MAX_PLAYERS)
        return;
    if (!state->players[id].active)
        return;
    if (input->player_id == -1)
        return;

    state->players[id].current_frame = input->current_frame;
    state->players[id].direction = input->direction;

    apply_movement(&state->players[id].x, &state->players[id].y, *input, dt);
}

int main(void)
{
    srand(time(NULL));

    UDPsocket server_socket = NULL;
    UDPpacket *receive_packet = NULL;
    UDPpacket *send_packet = NULL;

    gameState state = {0};
    state.type = MSG_GAME_STATE;
    state.phase = GAME_LOBBY;

    IPaddress clientAddresses[MAX_PLAYERS];
    int clientUsed[MAX_PLAYERS] = {0};

    // Spara senaste input per spelare
    clientInput lastInput[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        lastInput[i].player_id = -1;
    }

    if (!init_server(&server_socket))
        return 1;

    receive_packet = create_packet(PACKET_SIZE);
    if (!receive_packet)
    {
        cleanupServer(server_socket, NULL, NULL);
        return 1;
    }

    send_packet = create_packet(PACKET_SIZE);
    if (!send_packet)
    {
        cleanupServer(server_socket, receive_packet, NULL);
        return 1;
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    Uint64 lastBroadcast = SDL_GetPerformanceCounter();
    Uint64 state_start_time = 0;
    Uint64 phase_time = 0;

    while (1)
    {
        if (SDLNet_UDP_Recv(server_socket, receive_packet))
        {
            MessageType type;
            if (!packet_has_size(receive_packet, sizeof(MessageType), "MessageType"))
            {
                continue;
            }
            memcpy(&type, receive_packet->data, sizeof(MessageType));

            if (type == MSG_JOIN)
            {
                int existing = get_player_id_from_sender(clientAddresses, clientUsed, receive_packet->address);
                if (existing < 0)
                {
                    int newPlayer = addToLobby(&state, clientAddresses, clientUsed, receive_packet->address);
                    if (newPlayer >= 0)
                    {
                        printf("Player %d joined\n", newPlayer);
                        broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                    }
                }
            }
            else if (type == MSG_LEAVE)
            {
                int removed = removeFromLobby(&state, clientAddresses, clientUsed, receive_packet->address);
                if (removed >= 0)
                {
                    printf("Player %d left\n", removed);
                    broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                }
            }
            else if (type == MSG_START_GAME)
            {
                if (state.phase == GAME_LOBBY)
                {
                    int active_chosen_player = designateImpostor(&state);
                    printf("Player %d is impostor\n", active_chosen_player);
                    state.phase = GAME_SHOW_ROLE;
                    printf("Game is now GAME_SHOW_ROLE\n");

                    state_start_time = SDL_GetTicks64(); // TIDSSTÄMPEL

                    srand((unsigned int)time(NULL));
                    // The task list, one entry per TaskType (no TASK_NONE)
                    TaskType all_tasks[TASK_COUNT] = {
                        TASK_TIMER,
                        TASK_CLICK,
                        TASK_LETTER,
                        TASK_REFLEX,
                        TASK_LOGICAL_ORDER,
                        TASK_MEMORY};

                    for (int i = 0; i < MAX_PLAYERS; i++)
                    {
                        if (!state.players[i].active)
                            continue;

                        // Copy the base list then shuffle independently for each player
                        memcpy(state.players[i].task_order, all_tasks, sizeof(all_tasks));
                        shuffle_tasks(state.players[i].task_order, TASK_COUNT);
                        state.players[i].tasks_completed = 0;
                    }

                    printf("\n=== TASK ORDER ASSIGNMENT ===\n");
                    for (int i = 0; i < MAX_PLAYERS; i++)
                    {
                        if (!state.players[i].active)
                            continue;
                        printf("Player %d: ", i);
                        for (int j = 0; j < TASK_COUNT; j++)
                        {
                            printf("%d ", state.players[i].task_order[j]);
                        }
                        printf("\n");
                    }
                    printf("=============================\n");

                    state.total_tasks_completed = 0;

                    broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                }
            }
            else if (type == MSG_CLIENT_INPUT)
            {
                if (state.phase == GAME_RUNNING)
                {
                    clientInput input;

                    if (packet_has_size(receive_packet, sizeof(clientInput), "MSG_CLIENT_INPUT"))
                    {
                        memcpy(&input, receive_packet->data, sizeof(clientInput));
                        int sender_id = get_player_id_from_sender(clientAddresses, clientUsed, receive_packet->address);
                        if (sender_id >= 0)
                        {
                            input.player_id = sender_id;
                            lastInput[sender_id] = input;
                        }
                    }
                }
            }
            else if (type == MSG_KILL_REQUEST)
            {
                if (packet_has_size(receive_packet, sizeof(clientInput), "MSG_KILL_REQUEST"))
                {
                    int killer_id = get_player_id_from_sender(clientAddresses, clientUsed, receive_packet->address);
                    if (killer_id >= 0 && killer_id < MAX_PLAYERS)
                    {
                        int target_id = handle_kill_request(&state, killer_id);
                        if (target_id != -1)
                        {
                            state.players[target_id].isAlive = 0;
                            KillEventMsg msg = {0};
                            msg.type = MSG_KILL_EVENT;
                            msg.killer_id = killer_id;
                            msg.victim_id = target_id;
                            msg.x = state.players[killer_id].x;
                            msg.y = state.players[killer_id].y;
                            broadcast_Kill_msg(server_socket, send_packet, &msg, clientAddresses, clientUsed);
                        }
                    }
                }
            }
            else if (type == MSG_EMERGENCY_MEETING)
            {
                if (packet_has_size(receive_packet, sizeof(clientInput), "MSG_EMERGENCY_MEETING"))
                {
                    int local_id = get_player_id_from_sender(clientAddresses, clientUsed, receive_packet->address);
                    if (local_id >= 0 && local_id < MAX_PLAYERS && state.players[local_id].isAlive && state.players[local_id].emergency_meeting == 1)
                    {
                        state.phase = GAME_INFO_MEETING;
                        state.type = MSG_EMERGENCY_MEETING;
                        state.players[local_id].emergency_meeting = 0;
                        state.emergency_meeting_reported_id = local_id;
                        printf("[SERVER] Accept: player %d started an emergency meeting.\n", local_id);
                        phase_time = SDL_GetTicks64(); // TIDSSTÄMPEL
                        broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                    }
                }
            }
            else if (type == MSG_BODY_FOUND)
                if (packet_has_size(receive_packet, sizeof(clientInput), "MSG_BODY_FOUND"))
                {
                    int local_id = get_player_id_from_sender(clientAddresses, clientUsed, receive_packet->address);

                    clientInput report_body_request;
                    memcpy(&report_body_request, receive_packet->data, sizeof(report_body_request));

                    // printf("[SERVER] Received MSG_BODY_FOUND\n");
                    // printf("[SERVER] packet address matched local_id=%d\n", local_id);
                    // printf("[SERVER] request player_id=%d target_id=%d\n",
                    //        report_body_request.player_id,
                    //        report_body_request.target_id);

                    // printf("[SERVER] request dead_body=(%d, %.d)\n",
                    //        report_body_request.dead_body.x,
                    //        report_body_request.dead_body.y);

                    int target_id = report_body_request.target_id;
                    Position dead_body = report_body_request.dead_body;
                    int player_x = state.players[local_id].x;
                    int player_y = state.players[local_id].y;

                    // printf("[SERVER] player pos=(%d, %d), isAlive=%d\n",
                    //        player_x,
                    //        player_y,
                    //        state.players[local_id].isAlive);

                    if (local_id >= 0 && local_id < MAX_PLAYERS && state.players[local_id].isAlive && find_target_report_body(dead_body, player_x, player_y))
                    {
                        state.phase = GAME_INFO_MEETING;
                        state.type = MSG_BODY_FOUND;
                        state.emergency_meeting_reported_id = local_id;
                        printf("[SERVER] Accept: player %d found a body.\n", local_id);
                        phase_time = SDL_GetTicks64(); // TIDSSTÄMPEL
                        broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                    }
                }
                else if (type == MSG_TASK_COMPLETE)
                {
                    if (packet_has_size(receive_packet, sizeof(TaskCompleteMsg), "MSG_TASK_COMPLETE"))
                    {
                        TaskCompleteMsg msg;
                        memcpy(&msg, receive_packet->data, sizeof(msg));

                        // identify sender
                        int pid = get_player_id_from_sender(clientAddresses, clientUsed, receive_packet->address);
                        if (pid >= 0 && state.players[pid].active)
                        {
                            int completed = state.players[pid].tasks_completed;

                            // guard against out of bounds
                            if (completed < TASK_COUNT)
                            {
                                TaskType expected = state.players[pid].task_order[completed];

                                // Validate that the task type matches expected
                                if (msg.task_type == expected)
                                {
                                    // track completion
                                    state.players[pid].tasks_completed++;
                                    state.total_tasks_completed++;

                                    // calculate total tasks needed
                                    int active_count = countActivePlayers(&state);
                                    int total_expected_tasks = TASK_COUNT * (active_count - 1);

                                    printf("\n=== TASK COMPLETE ===\n");
                                    printf("Player %d finished task %d/%d (TaskType %d)\n", pid, state.players[pid].tasks_completed, TASK_COUNT, msg.task_type);
                                    printf("Team progress: %d/%d\n", state.total_tasks_completed, total_expected_tasks);
                                    printf("=====================\n");
                                }
                                else
                                {
                                    printf("[SERVER] Player %d sent wrong task type (got %d, expected %d)\n", pid, msg.task_type, expected);
                                }
                            }
                        }

                        broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
                    }
                }
        }

        // Applicera input och broadcasta på fast 60fps
        Uint64 now = SDL_GetPerformanceCounter();
        float broadcastDt = (float)(now - lastBroadcast) / (float)SDL_GetPerformanceFrequency();

        if (broadcastDt >= SERVER_TICK_INTERVAL)
        {
            if (state.phase == GAME_SHOW_ROLE)
            {
                if (SDL_GetTicks64() - state_start_time >= 3000) // NÄR 3 SEKUNDER GÅTT
                {
                    state.phase = GAME_RUNNING;
                    printf("Game is now GAME_RUNNING\n");
                }
                broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
            }
            else if (state.phase == GAME_INFO_MEETING)
            {
                if (SDL_GetTicks64() - phase_time >= 1500) // NÄR 3 SEKUNDER GÅTT
                {
                    state.phase = GAME_MEETING;
                    phase_time = 0;
                    phase_time = SDL_GetTicks64();
                    printf("INFORMATION OF MEETING ENDED\n");
                }
                broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
            }
            else if (state.phase == GAME_MEETING)
            {
                if (SDL_GetTicks64() - phase_time >= 10000) // NÄR 15 SEKUNDER GÅTT
                {
                    state.phase = GAME_RUNNING;
                    spawn_players(&state);
                    printf("MEETING ENDED\n");
                }
                broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
            }

            else if (state.phase == GAME_RUNNING)
            {
                for (int i = 0; i < MAX_PLAYERS; i++)
                {
                    handleInput(&state, &lastInput[i], 0.016f);
                    if (state.players[i].kill_cooldown_active)
                    {
                        state.players[i].kill_cooldown_active = update_kill_cooldown(state, i);
                    }
                    lastInput[i].player_id = -1;
                    lastInput[i].up = 0;
                    lastInput[i].down = 0;
                    lastInput[i].left = 0;
                    lastInput[i].right = 0;
                    lastInput[i].current_frame = 0;
                    lastInput[i].direction = DIR_DOWN;
                }
                broadcastGameState(server_socket, send_packet, &state, clientAddresses, clientUsed);
            }
            lastBroadcast = now;
        }
    }

    cleanupServer(server_socket, receive_packet, send_packet);
    return 0;
}
