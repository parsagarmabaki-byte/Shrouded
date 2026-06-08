#include <SDL2/SDL_net.h>
#include <stdio.h>
#include <string.h>
#include "network.h"
#include "network_data.h"
#include "game.h"
#include "imposter_ability.h"


int init_client(Client *client, const char *server_ip, char *error_message, size_t error_size)
{
    if (!init_network_socket(&client->socket, 0))
    {
        snprintf(error_message, error_size, "Could not open client socket");
        return 0;
    }

    if (SDLNet_ResolveHost(&client->serverAddr, server_ip, SERVER_PORT) != 0)
    {
        snprintf(error_message, error_size, "Invalid IP or unreachable host");
        printf("SDLNet_ResolveHost error: %s\n", SDLNet_GetError());
        SDLNet_UDP_Close(client->socket);
        client->socket = NULL;
        SDLNet_Quit();
        return 0;
    }

    IPaddress tcp_addr;
    if (SDLNet_ResolveHost(&tcp_addr, server_ip, SERVER_TCP_PORT) != 0)
    {
        snprintf(error_message, error_size, "Invalid TCP IP or unreachable host");
        printf("SDLNet_ResolveHost TCP error: %s\n", SDLNet_GetError());
        SDLNet_UDP_Close(client->socket);
        client->socket = NULL;
        SDLNet_Quit();
        return 0;
    }

    client->tcp_socket = SDLNet_TCP_Open(&tcp_addr);
    if (!client->tcp_socket)
    {
        snprintf(error_message, error_size, "Could not open TCP vote socket");
        printf("SDLNet_TCP_Open error: %s\n", SDLNet_GetError());
        SDLNet_UDP_Close(client->socket);
        client->socket = NULL;
        SDLNet_Quit();
        return 0;
    }

    client->tcp_socket_set = SDLNet_AllocSocketSet(1);
    if (!client->tcp_socket_set)
    {
        snprintf(error_message, error_size, "Could not allocate TCP vote socket set");
        SDLNet_TCP_Close(client->tcp_socket);
        client->tcp_socket = NULL;
        SDLNet_UDP_Close(client->socket);
        client->socket = NULL;
        SDLNet_Quit();
        return 0;
    }
    SDLNet_TCP_AddSocket(client->tcp_socket_set, client->tcp_socket);

    client->recievepacket = create_packet(1024);
    if (!client->recievepacket)
    {
        snprintf(error_message, error_size, "Could not allocate receive packet");
        SDLNet_FreeSocketSet(client->tcp_socket_set);
        client->tcp_socket_set = NULL;
        SDLNet_TCP_Close(client->tcp_socket);
        client->tcp_socket = NULL;
        SDLNet_UDP_Close(client->socket);
        client->socket = NULL;
        SDLNet_Quit();
        return 0;
    }

    error_message[0] = '\0';
    return 1;
}

void clean_client(Client *client)
{
    if (client->recievepacket)
    {
        SDLNet_FreePacket(client->recievepacket);
        client->recievepacket = NULL;
    }
    if (client->tcp_socket_set)
    {
        SDLNet_FreeSocketSet(client->tcp_socket_set);
        client->tcp_socket_set = NULL;
    }
    if (client->tcp_socket)
    {
        SDLNet_TCP_Close(client->tcp_socket);
        client->tcp_socket = NULL;
    }
    if (client->socket)
    {
        SDLNet_UDP_Close(client->socket);
        client->socket = NULL;
    }
    SDLNet_Quit();
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

int send_join(Client *client)
{
    joinMessage join = {0};
    join.type = MSG_JOIN;
    return send_client_message(client->socket, client->serverAddr, join.type);
}

int send_start_game(Client *client)
{
    startGameMessage start = {0};
    start.type = MSG_START_GAME;
    return send_client_message(client->socket, client->serverAddr, start.type);
}

int send_play_again(Client *client)
{
    return send_client_message(client->socket, client->serverAddr, MSG_PLAY_AGAIN);
}

int send_packet(UDPsocket socket, IPaddress server_addr, const void *data, size_t size)
{
    UDPpacket *packet = create_packet(550);
    if (!packet)
        return 0;

    if (!send_packet_data(socket, packet, server_addr, data, size))
    {
        SDLNet_FreePacket(packet);
        return 0;
    }

    SDLNet_FreePacket(packet);
    return 1;
}

int send_leave_message(Client *client)
{
    leaveMessage leave = {0};
    leave.type = MSG_LEAVE;

    UDPpacket *packet = create_packet(512);
    if (!packet)
    {
        return 0;
    }

    if (!send_packet_data(client->socket, packet, client->serverAddr, &leave, sizeof(leave)))
    {
        SDLNet_FreePacket(packet);
        return 0;
    }

    SDLNet_FreePacket(packet);
    return 1;
}
void send_input(Client *client, Player *player, InputMsg input_msg)
{
    input_msg.type = MSG_CLIENT_INPUT;
    input_msg.current_frame = player->current_frame;
    input_msg.direction = player->direction;
    send_packet(client->socket, client->serverAddr, &input_msg, sizeof(InputMsg));
}

int send_task_complete(Client *client, int player_id, TaskType task_type)
{
    TaskCompleteMsg msg = {0};
    msg.type = MSG_TASK_COMPLETE;
    msg.player_id = player_id;
    msg.task_type = task_type;

    UDPpacket *packet = create_packet(sizeof(msg));
    if (!packet)
        return 0;

    if (!send_packet_data(client->socket, packet, client->serverAddr, &msg, sizeof(msg)))
    {
        SDLNet_FreePacket(packet);
        return 0;
    }

    SDLNet_FreePacket(packet);
    return 1;
}

int send_debug_win(Client *client, MessageType type)
{
    if (type != MSG_DEBUG_CREWMATES_WIN && type != MSG_DEBUG_IMPOSTOR_WIN)
        return 0;

    UDPpacket *packet = create_packet(sizeof(type));
    if (!packet)
        return 0;

    if (!send_packet_data(client->socket, packet, client->serverAddr, &type, sizeof(type)))
    {
        SDLNet_FreePacket(packet);
        return 0;
    }

    SDLNet_FreePacket(packet);
    return 1;
}

void request_kill(Client *client, int target_id)
{
    printf("SENDING TARGET_ID %d", target_id);
    KillRequestMsg req = {0};
    req.type = MSG_KILL_REQUEST;
    req.target_id = target_id;
    send_packet(client->socket, client->serverAddr, &req, sizeof(KillRequestMsg));
}

void request_report_body(Client *client, int body_id)
{
    ReportBodyMsg input = {0};
    input.type = MSG_BODY_FOUND;
    input.body_id = body_id;
    send_packet(client->socket, client->serverAddr, &input, sizeof(ReportBodyMsg));
}

void request_emergency_meeting(Client *client, gameState *state, int local_id)
{
    EmergencyMeetingMsg request = {0};
    request.type = MSG_EMERGENCY_MEETING;
    send_packet(client->socket, client->serverAddr, &request, sizeof(EmergencyMeetingMsg));
}

void send_vote(Client *client, int targeted_banner, int voter_id)
{
    if (!client->tcp_socket)
        return;

    VoteRequest vote = {0};
    vote.type = MSG_VOTE_REQUEST;
    vote.target_id = targeted_banner;
    vote.voter_id = voter_id;
    send_tcp_data(client->tcp_socket, &vote, sizeof(VoteRequest));
}

static void apply_vote_update(gameState *state, const VoteUpdateMsg *msg, int *player_voted)
{
    state->phase = msg->phase;
    state->meeting_reason = msg->meeting_reason;
    state->emergency_meeting_reported_id = msg->emergency_meeting_reported_id;
    state->voting_result = msg->voting_result;

    for (int i = 0; i < MAX_PLAYERS + 1; i++)
        state->voting_results[i] = msg->voting_results[i];
}

static void collect_tcp_vote_packets(Client *client, gameState *state, int *player_voted)
{
    if (!client->tcp_socket || !client->tcp_socket_set)
        return;

    int ready = SDLNet_CheckSockets(client->tcp_socket_set, 0);
    if (ready <= 0 || !SDLNet_SocketReady(client->tcp_socket))
        return;

    int remaining = (int)sizeof(VoteUpdateMsg) - client->vote_update_bytes_read;
    int received = SDLNet_TCP_Recv(client->tcp_socket,
                                   ((char *)&client->vote_update_buffer) + client->vote_update_bytes_read,
                                   remaining);
    f (received < 0)
{
    // Verkligt fel — stäng
    SDLNet_TCP_DelSocket(...); SDLNet_TCP_Close(...);
    client->tcp_socket = NULL;
    client->vote_update_bytes_read = 0;
    return;
}
if (received == 0)
    return; // Inget data ännu — behåll bufferten

    client->vote_update_bytes_read += received;
    if (client->vote_update_bytes_read == (int)sizeof(VoteUpdateMsg))
    {
        if (client->vote_update_buffer.type == MSG_VOTE_UPDATE)
            apply_vote_update(state, &client->vote_update_buffer, player_voted);
        client->vote_update_bytes_read = 0;
    }
}

static void apply_phase_change_msg(PhaseChangeMsg *msg, gameState *state,
                                   AudioAssets *audio,
                                   KillAnimation bodies[MAX_PLAYERS],
                                   int *targeted_banner,
                                   int *player_voted)
{
    if (msg->type == MSG_PHASE_CHANGE)
        state->phase = msg->phase;

    else if (msg->type == MSG_TASK_COMPLETE)
    {
        state->total_tasks_completed++;
        state->players[msg->player_id].tasks_completed++;
    }

    else if (msg->type == MSG_EMERGENCY_MEETING || msg->type == MSG_BODY_FOUND)
    {
        *targeted_banner = -1;
        *player_voted = -1;
        state->phase = msg->phase;
        state->meeting_reason = msg->meeting_reason;
        state->emergency_meeting_reported_id = msg->player_id;

        if (msg->type == MSG_EMERGENCY_MEETING)
            state->players[msg->player_id].emergency_meeting = 0;

        play_meeting_horn(audio);
    }

    else if (msg->type == MSG_KILL_EVENT)
    {
        state->players[msg->victim_id].isAlive = 0;
        state->kill_cooldown_active = true;
        state->phase = msg->phase;

        if (bodies)
        {
            start_kill_animation(&bodies[msg->victim_id], msg->player_id, msg->victim_id,
                                 state->players[msg->victim_id].x,
                                 state->players[msg->victim_id].y);

            printf("\nCLIENT %d START BODY victim=%d active=%d x=%.1f y=%.1f\n",
                   state->local_player_id, msg->victim_id,
                   bodies[msg->victim_id].active,
                   bodies[msg->victim_id].x,
                   bodies[msg->victim_id].y);
        }

        if (state->local_player_id == msg->player_id ||
            state->local_player_id == msg->victim_id)
        {
            play_kill_knife(audio);
            play_dramatic_kill(audio);
        }
    }
    else if (msg->type == MSG_MEETING_ENDED)
    {
        if (state->voting_result != -1)
            state->players[state->voting_result].isAlive = 0;
        state->phase = msg->phase;
    }
    else if (msg->type == MSG_KILL_READY)
        state->kill_cooldown_active = false;
}

static void collect_event_packets(Client *client, gameState *state, AudioAssets *audio, KillAnimation bodies[MAX_PLAYERS], int *targeted_banner, int *player_voted)
{
    if (!client->tcp_socket || !client->tcp_socket_set)
        return;

    if (SDLNet_CheckSockets(client->tcp_socket_set, 0) <= 0 ||
        !SDLNet_SocketReady(client->tcp_socket))
        return;

    int remaining = sizeof(PhaseChangeMsg) - client->phase_change_bytes_read;

    int received = SDLNet_TCP_Recv(client->tcp_socket,
                                   (char *)&client->phase_change_buffer + client->phase_change_bytes_read,
                                   remaining);

    if (received < 0)
    {
        client->phase_change_bytes_read = 0;
        return;
    }
    if (received == 0)
        return;

    client->phase_change_bytes_read += received;

    if (client->phase_change_bytes_read == sizeof(PhaseChangeMsg))
    {
        apply_phase_change_msg(&client->phase_change_buffer, state, audio, bodies, targeted_banner, player_voted);
        client->phase_change_bytes_read = 0;
    }
}


void collect_packets(Client *client, gameState *state, KillAnimation *bodies, AudioAssets *audio, int *targeted_banner, int *player_voted)
{
    if (state->phase == GAME_MEETING)
        collect_tcp_vote_packets(client, state, player_voted);
    else 
        collect_event_packets(client, state, audio, bodies,targeted_banner, player_voted);

    while (SDLNet_UDP_Recv(client->socket, client->recievepacket))
    {
        if (!packet_has_size(client->recievepacket, sizeof(MessageType), "MessageType"))
        {
            continue;
        }

        MessageType type;
        memcpy(&type, client->recievepacket->data, sizeof(type));

        if (type == MSG_GAME_STATE)
        {
            if (packet_has_size(client->recievepacket, sizeof(gameState), "MSG_GAME_STATE"))
            {
                memcpy(state, client->recievepacket->data, sizeof(gameState));
            }
        }
        else if (type == MSG_PLAYER_SYNC_DATA)
        {
            PlayerSyncMsg msg = {0};
            if (packet_has_size(client->recievepacket, sizeof(PlayerSyncMsg), "PlayerSyncMsg"))
            {
                memcpy(&msg, client->recievepacket->data, sizeof(PlayerSyncMsg));
                for (int i = 0; i < MAX_PLAYERS; i++)
                {
                    state->players[i].x = msg.player[i].x;
                    state->players[i].y = msg.player[i].y;
                    state->players[i].direction = msg.player[i].direction;
                    state->players[i].current_frame = msg.player[i].current_frame;
                }
            }
        }
        else if (type == MSG_MEETING_TIMER)
        {
            MeetingTimer msg = {0};
            if (packet_has_size(client->recievepacket, sizeof(MeetingTimer), "MeetingTimer"))
            {
                memcpy(&msg, client->recievepacket->data, sizeof(MeetingTimer));
                state->meeting_time_remaining = msg.meeting_time_remaining;
            }
        }
    }
}
