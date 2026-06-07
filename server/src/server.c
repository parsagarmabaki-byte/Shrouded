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
void handle_start_game(Server *s, IPaddress sender);
void handle_play_again(Server *s);
void handle_client_input(Server *s, IPaddress sender);
void handle_kill(Server *s, IPaddress sender);
void handle_emergency_meeting(Server *s, IPaddress sender);
void handle_body_found(Server *s, IPaddress sender);
void handle_task_complete(Server *s, IPaddress sender);
void handle_vote(Server *s, IPaddress sender, gameState *state);

// ===================== MAIN =====================

int main(void)
{
    srand(time(NULL));

    Server server = {0};

    server.state.type = MSG_GAME_STATE;
    server.state.phase = GAME_LOBBY;
    server.state.host_player_id = -1;
    server.state.meeting_reason = MEETING_NONE;
    server.state.emergency_meeting_reported_id = -1;
    server.last_timer_second = -1;

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
            case MSG_JOIN:
                handle_join(&server, sender);
                break;
            case MSG_LEAVE:
                handle_leave(&server, sender);
                break;
            case MSG_START_GAME:
                handle_start_game(&server, sender);
                break;
            case MSG_PLAY_AGAIN:
                handle_play_again(&server);
                break;
            case MSG_CLIENT_INPUT:
                handle_client_input(&server, sender);
                break;
            case MSG_KILL_REQUEST:
                handle_kill(&server, sender);
                break;
            case MSG_EMERGENCY_MEETING:
                handle_emergency_meeting(&server, sender);
                break;
            case MSG_BODY_FOUND:
                handle_body_found(&server, sender);
                break;
            case MSG_TASK_COMPLETE:
                handle_task_complete(&server, sender);
                break;
            case MSG_VOTE_REQUEST:
                handle_vote(&server, sender, &server.state);
                break;
            case MSG_DEBUG_CREWMATES_WIN:
#ifdef DEBUG
                server.state.phase = GAME_CREWMATES_WIN;
#endif
                break;
            case MSG_DEBUG_IMPOSTOR_WIN:
#ifdef DEBUG
                server.state.phase = GAME_KILLER_WIN;
#endif
                break;
            default:
                break;
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
            PhaseChangeMsg msg = {0};
            msg.phase = GAME_RUNNING;
            msg.type = MSG_PHASE_CHANGE;
            broadcast_msg(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &msg, sizeof(PhaseChangeMsg));
        }
    }
    else if (s->state.phase == GAME_INFO_MEETING)
    {
        if (SDL_GetTicks64() - s->phase_time >= INFO_MEETING_DURATION)
        {
            s->state.phase = GAME_MEETING;
            inititate_meeting_info(&s->meeting_info, &s->state);
            s->phase_time = SDL_GetTicks64();
            printf("INFORMATION OF MEETING ENDED\n");

            PhaseChangeMsg msg = {0};
            msg.phase = GAME_MEETING;
            msg.type = MSG_PHASE_CHANGE;
            broadcast_msg(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &msg, sizeof(PhaseChangeMsg));
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
            s->state.voting_result = resolve_voting(&s->state, s->meeting_info, s->state.voting_results);
            s->state.meeting_reason = MEETING_NONE;
            printf("MEETING ENDED\n");

            MeetingEndedEvent msg = {0};
            msg.phase = SHOW_VOTE_RESULT;
            for (int i = 0; i < 7; i++)
                msg.voting_results[i] = s->state.voting_results[i];
            msg.type = MSG_MEETING_ENDED;
            msg.voting_result = s->state.voting_result;
            msg.meeting_reason = MEETING_NONE;
            broadcast_msg(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &msg, sizeof(MeetingEndedEvent));
            s->last_timer_second = -1;
            s->phase_time = SDL_GetTicks64();
        }
        int current_second = (int) (s->state.meeting_time_remaining / 1000);
        if (current_second != s->last_timer_second)
        {
            s->last_timer_second = current_second;
            MeetingTimer timer_msg;
            timer_msg.type = MSG_MEETING_TIMER;
            timer_msg.meeting_time_remaining = s->state.meeting_time_remaining;
            broadcast_msg(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &timer_msg, sizeof(MeetingTimer));
        }
    }
    else if (s->state.phase == SHOW_VOTE_RESULT)
    {
        if (SDL_GetTicks64() - s->phase_time >= VOTE_RESULT_DURATION)
        {
            s->state.phase = GAME_RUNNING;
            for (int i = 0; i < MAX_PLAYERS; i++)
                s->deadBodyActive[i] = 0;
            spawn_players(&s->state);
            check_win_condition(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &s->state);
            broadcast_game_state(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
        }
    }
    else if (s->state.phase == GAME_RUNNING)
    {
        PlayerSyncMsg msg = {0};
        msg.type = MSG_PLAYER_SYNC_DATA;
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            apply_player_input(&s->state, &s->lastInput[i], SERVER_TICK_INTERVAL);

            msg.player[i].x = s->state.players[i].x;
            msg.player[i].y = s->state.players[i].y;
            msg.player[i].direction = s->state.players[i].direction;
            msg.player[i].player_id = i;
            msg.player[i].current_frame = s->state.players[i].current_frame;
        }
        if (s->kill_cooldown_start != 0)
            update_kill_cooldown(s->socket, s->send_packet, s->clientAddresses[s->killer_id], &s->kill_cooldown_start, &s->state.kill_cooldown_active);
        broadcast_msg(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &msg, sizeof(PlayerSyncMsg));
    }
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
            if (s->state.host_player_id < 0)
                s->state.host_player_id = newPlayer;
            printf("Player %d joined\n", newPlayer);
            broadcast_game_state(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
        }
    }
}

void handle_leave(Server *s, IPaddress sender)
{
    int removed = removeFromLobby(&s->state, s->clientAddresses, s->clientUsed, sender);
    if (removed >= 0)
    {
        printf("Player %d left\n", removed);
        if (s->state.host_player_id == removed)
        {
            s->state.host_player_id = -1;
            for (int i = 0; i < MAX_PLAYERS; i++)
            {
                if (s->clientUsed[i])
                {
                    s->state.host_player_id = i;
                    break;
                }
            }
        }
        broadcast_game_state(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_start_game(Server *s, IPaddress sender)
{
    int sender_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (sender_id != s->state.host_player_id)
    {
        printf("[SERVER] Ignored start request from non-host player %d\n", sender_id);
        return;
    }
    if (countActivePlayers(&s->state) < 2)
    {
        printf("[SERVER] Ignored start request: at least 2 players are required\n");
        return;
    }
    if (s->state.phase == GAME_LOBBY)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
            s->deadBodyActive[i] = 0;
        start_new_round(&s->state, &s->state_start_time, &s->killer_id);
        printf("Game is now GAME_SHOW_ROLE\n");
        broadcast_game_state(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_play_again(Server *s)
{
    if (s->state.phase == GAME_CREWMATES_WIN || s->state.phase == GAME_KILLER_WIN)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
            s->deadBodyActive[i] = 0;
        start_new_round(&s->state, &s->state_start_time, &s->killer_id);
        printf("Play again: game is now GAME_SHOW_ROLE\n");
        broadcast_game_state(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_client_input(Server *s, IPaddress sender)
{
    if (s->state.phase != GAME_RUNNING)
        return;

    if (packet_has_size(s->receive_packet, sizeof(InputMsg), "MSG_CLIENT_INPUT"))
    {
        InputMsg input_msg;
        memcpy(&input_msg, s->receive_packet->data, sizeof(InputMsg));
        int sender_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
        if (sender_id >= 0)
        {
            input_msg.player_id = sender_id;
            s->lastInput[sender_id] = input_msg;
        }
    }
}

void handle_kill(Server *s, IPaddress sender)
{
    int killer_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    printf("\nRECIEVED KILL REQUEST FROM KILL ID %d\n", killer_id);

    if (s->state.phase != GAME_RUNNING)
        return;

    if (!packet_has_size(s->receive_packet, sizeof(KillRequestMsg), "MSG_KILL_REQUEST"))
        return;

    if (killer_id < 0 || killer_id >= MAX_PLAYERS)
        return;

    if (!s->state.players[killer_id].active || !s->state.players[killer_id].isAlive || !s->state.players[killer_id].isImpostor)
        return;

    KillRequestMsg request;
    memcpy(&request, s->receive_packet->data, sizeof(request));
    int target_id = handle_kill_request(&s->state, killer_id);
    printf("\nCLOSEST PLAYER TO THE PLAYER IS %d and PLAYER WANT TO KILL ID %d", target_id, request.target_id);

    if (request.target_id == target_id)
    {
        s->state.players[target_id].isAlive = 0;
        s->deadBodies[target_id].x = (int)s->state.players[target_id].x;
        s->deadBodies[target_id].y = (int)s->state.players[target_id].y;
        s->deadBodyActive[target_id] = 1;

        KillEventMsg msg = {0};
        msg.type = MSG_KILL_EVENT;
        msg.killer_id = killer_id;
        msg.victim_id = target_id;
        msg.x = s->state.players[killer_id].x;
        msg.y = s->state.players[killer_id].y;

        activate_kill_cooldown(&s->kill_cooldown_start, &s->state.kill_cooldown_active);
        broadcast_msg(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &msg, sizeof(KillEventMsg));
        check_win_condition(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &s->state);
    }
}

void handle_emergency_meeting(Server *s, IPaddress sender)
{
    if (s->state.phase != GAME_RUNNING)
        return;

    if (!packet_has_size(s->receive_packet, sizeof(EmergencyMeetingMsg), "MSG_EMERGENCY_MEETING"))
        return;

    int sender_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (sender_id >= 0 && sender_id < MAX_PLAYERS && s->state.players[sender_id].isAlive && s->state.players[sender_id].emergency_meeting == 1)
    {
        s->state.phase = GAME_INFO_MEETING;
        s->state.type = MSG_EMERGENCY_MEETING;
        s->state.meeting_reason = MEETING_EMERGENCY;
        s->state.players[sender_id].emergency_meeting = 0;
        s->state.emergency_meeting_reported_id = sender_id;

        EmergencyMeetingEvent msg = {0};
        msg.phase = GAME_INFO_MEETING;
        msg.type = MSG_EMERGENCY_MEETING;
        msg.meeting_reason = MEETING_EMERGENCY;
        msg.emergency_meeting_reported_id = sender_id;

        printf("[SERVER] Accept: player %d started an emergency meeting.\n", sender_id);
        s->phase_time = SDL_GetTicks64();
        broadcast_msg(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &msg, sizeof(EmergencyMeetingEvent));
    }
}

void handle_body_found(Server *s, IPaddress sender)
{
    if (s->state.phase != GAME_RUNNING)
        return;

    if (!packet_has_size(s->receive_packet, sizeof(ReportBodyMsg), "MSG_BODY_FOUND"))
        return;

    int reported_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (reported_id < 0 || reported_id >= MAX_PLAYERS)
        return;

    ReportBodyMsg report;
    memcpy(&report, s->receive_packet->data, sizeof(report));

    int body_id = report.body_id;
    if (body_id < 0 || body_id >= MAX_PLAYERS || !s->deadBodyActive[body_id])
        return;

    Position dead_body = s->deadBodies[body_id];
    float player_x = s->state.players[reported_id].x;
    float player_y = s->state.players[reported_id].y;

    if (s->state.players[reported_id].isAlive && find_target_report_body(dead_body, player_x, player_y))
    {
        s->state.phase = GAME_INFO_MEETING;
        s->state.type = MSG_BODY_FOUND;
        s->state.meeting_reason = MEETING_BODY;
        s->state.emergency_meeting_reported_id = reported_id;

        EmergencyMeetingEvent msg = {0};
        msg.phase = GAME_INFO_MEETING;
        msg.type = MSG_BODY_FOUND;
        msg.meeting_reason = MEETING_BODY;
        msg.emergency_meeting_reported_id = reported_id;

        printf("[SERVER] Accept: player %d found a body.\n", reported_id);
        s->phase_time = SDL_GetTicks64();
        broadcast_msg(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &msg, sizeof(EmergencyMeetingEvent));
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
                check_win_condition(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &s->state);

                TaskCompletedEvent task_completed_msg = {0};
                task_completed_msg.type = MSG_TASK_COMPLETE;
                task_completed_msg.player_id = pid;

                int active_count = countActivePlayers(&s->state);
                int total_expected_tasks = TASK_COUNT * (active_count - 1);
                printf("\n=== TASK COMPLETE ===\n");
                printf("Player %d finished task %d/%d (TaskType %d)\n", pid, s->state.players[pid].tasks_completed, TASK_COUNT, msg.task_type);
                printf("Team progress: %d/%d\n", s->state.total_tasks_completed, total_expected_tasks);
                printf("=====================\n");
                broadcast_msg(s->socket, s->send_packet, s->clientAddresses, s->clientUsed, &task_completed_msg, sizeof(TaskCompletedEvent));
            }
            else
            {
                printf("[SERVER] Player %d sent wrong task type (got %d, expected %d)\n", pid, msg.task_type, expected);
            }
        }
    }
}

void handle_vote(Server *s, IPaddress sender, gameState *state)
{
    if (state->phase != GAME_MEETING)
        return;

    if (!packet_has_size(s->receive_packet, sizeof(VoteRequest), "VoteRequest"))
        return;

    VoteRequest vote;
    memcpy(&vote, s->receive_packet->data, sizeof(vote));
    int pid = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    vote.voter_id = pid;
    if (vote.target_id != VOTE_SKIP && (vote.target_id < 0 || vote.target_id >= MAX_PLAYERS))
        return;
    if (vote.target_id >= 0 && (!state->players[vote.target_id].active || !state->players[vote.target_id].isAlive))
        return;
    if (can_cast_vote(s->meeting_info, pid) && s->meeting_info.votes_recieved < s->meeting_info.alive_players_count)
    {
        cast_vote(&s->meeting_info, vote);
    }
}
