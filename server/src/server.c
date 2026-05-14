#define SDL_MAIN_HANDLED
#include <SDL2/SDL_net.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "network.h"
#include "network_data.h"
#include "imposter_ability.h"
#include "server_context.h"
#include "server_broadcast.h"
#include "server_lobby.h"
#include "server_round.h"
#include "server_meeting.h"
#include "server_game_logic.h"

#define PACKET_SIZE 1024

// Forward declarations
void update_server_tick(Server *s);
void handle_join(Server *s, IPaddress sender);
void handle_leave(Server *s, IPaddress sender);
void handle_start_game(Server *s);
void handle_play_again(Server *s);
void handle_client_input(Server *s, IPaddress sender);
void handle_kill(Server *s, IPaddress sender);
void handle_emergency_meeting(Server *s, IPaddress sender);
void handle_body_found(Server *s, IPaddress sender);
void handle_task_complete(Server *s, IPaddress sender);
void handle_vote(Server *s, IPaddress sender);

// ===================== MAIN =====================

int main(void)
{
    srand(time(NULL));

    Server server = {0};

    server.state.type = MSG_GAME_STATE;
    server.state.phase = GAME_LOBBY;

    for (int i = 0; i < MAX_PLAYERS; i++)
        server.lastInput[i].player_id = -1;

    if (!init_server_socket(&server.socket))
        return 1;

    server.receive_packet = create_packet(PACKET_SIZE);
    if (!server.receive_packet)
    {
        cleanupServer(server.socket, NULL, NULL);
        return 1;
    }

    server.send_packet = create_packet(PACKET_SIZE);
    if (!server.send_packet)
    {
        cleanupServer(server.socket, server.receive_packet, NULL);
        return 1;
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    Uint64 lastBroadcast = SDL_GetPerformanceCounter();

    while (1)
    {
        if (SDLNet_UDP_Recv(server.socket, server.receive_packet))
        {
            MessageType type;

            if (!packet_has_size(server.receive_packet, sizeof(MessageType), "MessageType"))
                continue;
            memcpy(&type, server.receive_packet->data, sizeof(MessageType));
            IPaddress sender = server.receive_packet->address;

            switch (type)
            {
                case MSG_JOIN:                  handle_join(&server, sender);              break;
                case MSG_LEAVE:                 handle_leave(&server, sender);             break;
                case MSG_START_GAME:            handle_start_game(&server);                break;
                case MSG_PLAY_AGAIN:            handle_play_again(&server);                break;
                case MSG_CLIENT_INPUT:          handle_client_input(&server, sender);      break;
                case MSG_KILL_REQUEST:          handle_kill(&server, sender);              break;
                case MSG_EMERGENCY_MEETING:     handle_emergency_meeting(&server, sender); break;
                case MSG_BODY_FOUND:            handle_body_found(&server, sender);        break;
                case MSG_TASK_COMPLETE:         handle_task_complete(&server, sender);     break;
                case MSG_VOTE_REQUEST:          handle_vote(&server, sender);              break;
                case MSG_DEBUG_CREWMATES_WIN:   server.state.phase = GAME_CREWMATES_WIN;   break;
                case MSG_DEBUG_IMPOSTOR_WIN:    server.state.phase = GAME_IMPOSTOR_WIN;    break;
                default: break;
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float broadcastDt = (float)(now - lastBroadcast) / (float)SDL_GetPerformanceFrequency();

        if (broadcastDt >= SERVER_TICK_INTERVAL)
        {
            update_server_tick(&server);
            lastBroadcast = now;
        }
    }

    cleanupServer(server.socket, server.receive_packet, server.send_packet);
    return 0;
}

// ===================== SERVER TICK =====================

void update_server_tick(Server *s)
{
    if (s->state.phase == GAME_SHOW_ROLE)
    {
        if (SDL_GetTicks64() - s->state_start_time >= SHOW_ROLE_DURATION)
        {
            s->state.phase = GAME_RUNNING;
            printf("Game is now GAME_RUNNING\n");
        }
    }
    else if (s->state.phase == GAME_INFO_MEETING)
    {
        if (SDL_GetTicks64() - s->phase_time >= INFO_MEETING_DURATION)
        {
            s->state.phase = GAME_MEETING;
            inititate_meeting_info(&s->meeting_info, s->state);
            s->phase_time = SDL_GetTicks64();
            printf("INFORMATION OF MEETING ENDED\n");
        }
    }
    else if (s->state.phase == GAME_MEETING)
    {
        Uint32 elapsed = SDL_GetTicks64() - s->phase_time;
        Uint32 meeting_duration = MEETING_DURATION;
        s->state.meeting_time_remaining = (elapsed < meeting_duration)
            ? (int)(meeting_duration - elapsed)
            : 0;

        if (elapsed >= meeting_duration || s->meeting_info.votes_recieved == s->meeting_info.alive_players_count)
        {
            s->state.phase = SHOW_VOTE_RESULT;
            s->state.voting_result = calculate_votes(s->meeting_info, s->state.voting_results);
            printf("MEETING ENDED\n");
            s->phase_time = SDL_GetTicks64();
        }
    }
    else if (s->state.phase == SHOW_VOTE_RESULT)
    {
        if (SDL_GetTicks64() - s->phase_time >= VOTE_RESULT_DURATION)
        {
            resolve_voting(&s->state, s->meeting_info, s->state.voting_results);
            spawn_players(&s->state);
            s->state.phase = GAME_RUNNING;
            check_win_condition(&s->state);
        }
    }
    else if (s->state.phase == GAME_RUNNING)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            apply_player_input(&s->state, &s->lastInput[i], SERVER_TICK_INTERVAL);
            if (s->state.players[i].kill_cooldown_active)
            {
                s->state.players[i].kill_cooldown_active = update_kill_cooldown(s->state, i);
            }
            s->lastInput[i].player_id = -1;
            s->lastInput[i].up = 0;
            s->lastInput[i].down = 0;
            s->lastInput[i].left = 0;
            s->lastInput[i].right = 0;
            s->lastInput[i].current_frame = 0;
            s->lastInput[i].direction = DIR_DOWN;
        }
    }

    broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
}

// ===================== MESSAGE HANDLERS =====================

void handle_join(Server *s, IPaddress sender)
{
    int existing = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (existing < 0)
    {
        int newPlayer = addToLobby(&s->state, s->clientAddresses, s->clientUsed, sender);
        if (newPlayer >= 0)
        {
            printf("Player %d joined\n", newPlayer);
            broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
        }
    }
}

void handle_leave(Server *s, IPaddress sender)
{
    int removed = removeFromLobby(&s->state, s->clientAddresses, s->clientUsed, sender);
    if (removed >= 0)
    {
        printf("Player %d left\n", removed);
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_start_game(Server *s)
{
    if (s->state.phase == GAME_LOBBY)
    {
        start_new_round(&s->state, &s->state_start_time);
        printf("Game is now GAME_SHOW_ROLE\n");
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_play_again(Server *s)
{
    if (s->state.phase == GAME_CREWMATES_WIN || s->state.phase == GAME_IMPOSTOR_WIN)
    {
        start_new_round(&s->state, &s->state_start_time);
        printf("Play again: game is now GAME_SHOW_ROLE\n");
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_client_input(Server *s, IPaddress sender)
{
    if (s->state.phase != GAME_RUNNING)
        return;

    if (packet_has_size(s->receive_packet, sizeof(clientInput), "MSG_CLIENT_INPUT"))
    {
        clientInput input;
        memcpy(&input, s->receive_packet->data, sizeof(clientInput));
        int sender_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
        if (sender_id >= 0)
        {
            input.player_id = sender_id;
            s->lastInput[sender_id] = input;
        }
    }
}

void handle_kill(Server *s, IPaddress sender)
{
    if (!packet_has_size(s->receive_packet, sizeof(clientInput), "MSG_KILL_REQUEST"))
        return;

    int killer_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (killer_id < 0 || killer_id >= MAX_PLAYERS)
        return;

    int target_id = handle_kill_request(&s->state, killer_id);
    if (target_id != -1)
    {
        s->state.players[target_id].isAlive = 0;
        KillEventMsg msg = {0};
        msg.type = MSG_KILL_EVENT;
        msg.killer_id = killer_id;
        msg.victim_id = target_id;
        msg.x = s->state.players[killer_id].x;
        msg.y = s->state.players[killer_id].y;
        broadcast_Kill_msg(s->socket, s->send_packet, &msg, s->clientAddresses, s->clientUsed);
        check_win_condition(&s->state);
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_emergency_meeting(Server *s, IPaddress sender)
{
    if (!packet_has_size(s->receive_packet, sizeof(clientInput), "MSG_EMERGENCY_MEETING"))
        return;

    int local_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (local_id >= 0 && local_id < MAX_PLAYERS && s->state.players[local_id].isAlive && s->state.players[local_id].emergency_meeting == 1)
    {
        s->state.phase = GAME_INFO_MEETING;
        s->state.type = MSG_EMERGENCY_MEETING;
        s->state.players[local_id].emergency_meeting = 0;
        s->state.emergency_meeting_reported_id = local_id;
        printf("[SERVER] Accept: player %d started an emergency meeting.\n", local_id);
        s->phase_time = SDL_GetTicks64();
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_body_found(Server *s, IPaddress sender)
{
    if (!packet_has_size(s->receive_packet, sizeof(clientInput), "MSG_BODY_FOUND"))
        return;

    int local_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (local_id < 0 || local_id >= MAX_PLAYERS)
        return;

    clientInput report;
    memcpy(&report, s->receive_packet->data, sizeof(report));

    Position dead_body = report.dead_body;
    int player_x = s->state.players[local_id].x;
    int player_y = s->state.players[local_id].y;

    if (s->state.players[local_id].isAlive && find_target_report_body(dead_body, player_x, player_y))
    {
        s->state.phase = GAME_INFO_MEETING;
        s->state.type = MSG_BODY_FOUND;
        s->state.emergency_meeting_reported_id = local_id;
        printf("[SERVER] Accept: player %d found a body.\n", local_id);
        s->phase_time = SDL_GetTicks64();
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_task_complete(Server *s, IPaddress sender)
{
    if (!packet_has_size(s->receive_packet, sizeof(TaskCompleteMsg), "MSG_TASK_COMPLETE"))
        return;

    TaskCompleteMsg msg;
    memcpy(&msg, s->receive_packet->data, sizeof(msg));

    int pid = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (pid >= 0 && s->state.phase == GAME_RUNNING && s->state.players[pid].active && !s->state.players[pid].isImpostor)
    {
        int completed = s->state.players[pid].tasks_completed;
        if (completed < TASK_COUNT)
        {
            TaskType expected = s->state.players[pid].task_order[completed];
            if (msg.task_type == expected)
            {
                s->state.players[pid].tasks_completed++;
                s->state.total_tasks_completed++;
                check_win_condition(&s->state);

                int active_count = countActivePlayers(&s->state);
                int total_expected_tasks = TASK_COUNT * (active_count - 1);
                printf("\n=== TASK COMPLETE ===\n");
                printf("Player %d finished task %d/%d (TaskType %d)\n", pid, s->state.players[pid].tasks_completed, TASK_COUNT, msg.task_type);
                printf("Team progress: %d/%d\n", s->state.total_tasks_completed, total_expected_tasks);
                printf("=====================\n");
            }
            else
            {
                printf("[SERVER] Player %d sent wrong task type (got %d, expected %d)\n", pid, msg.task_type, expected);
            }
        }
    }
    broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
}

void handle_vote(Server *s, IPaddress sender)
{
    if (!packet_has_size(s->receive_packet, sizeof(VoteRequest), "VoteRequest"))
        return;

    VoteRequest vote;
    memcpy(&vote, s->receive_packet->data, sizeof(vote));
    int pid = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    vote.voter_id = pid;
    if (can_cast_vote(s->meeting_info, pid) && s->meeting_info.votes_recieved < s->meeting_info.alive_players_count)
        cast_vote(&s->meeting_info, vote);
}