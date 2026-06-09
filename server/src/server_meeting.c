#include "server_meeting.h"
#include "server_broadcast.h"
#include <stdio.h>

void handle_leave_by_id(Server *s, int player_id);

void initiate_meeting_info(Meeting *meeting_info, GameState *state)
{
    meeting_info->alive_players_count = 0;
    meeting_info->votes_received = 0;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        meeting_info->alive_players_id[i] = -1;
        meeting_info->votes[i].voter_id = 0;
        meeting_info->votes[i].target_id = -1;
        meeting_info->has_voted[i] = 0;
    }

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!state->players[i].isAlive || !state->players[i].active)
            continue;

        meeting_info->alive_players_id[meeting_info->alive_players_count] = i;
        meeting_info->alive_players_count++;
    }
}

int can_cast_vote(Meeting meeting_info, int voter_id)
{
    if (voter_id < 0 || voter_id >= MAX_PLAYERS)
        return 0;

    if (meeting_info.has_voted[voter_id])
        return 0;

    int is_alive_voter = 0;
    for (int i = 0; i < meeting_info.alive_players_count; i++)
    {
        if (meeting_info.alive_players_id[i] == voter_id)
        {
            is_alive_voter = 1;
            break;
        }
    }
    if (!is_alive_voter)
        return 0;

    return 1;
}

void cast_vote(Meeting *meeting_info, VoteRequest vote)
{
    if (meeting_info->votes_received < 0 || meeting_info->votes_received >= MAX_PLAYERS)
        return;
    if (vote.voter_id < 0 || vote.voter_id >= MAX_PLAYERS)
        return;
    if (vote.target_id != VOTE_SKIP && (vote.target_id < 0 || vote.target_id >= MAX_PLAYERS))
        return;

    int index = meeting_info->votes_received;
    meeting_info->votes[index] = vote;
    meeting_info->has_voted[vote.voter_id] = 1;
    meeting_info->votes_received++;
}

int calculate_votes(Meeting meeting_info, int voting_result[MAX_PLAYERS + 1])
{
    int votes_received = meeting_info.votes_received;
    int max_votes = 0;
    int player_id = -1;

    for (int i = 0; i < MAX_PLAYERS + 1; i++)
        voting_result[i] = 0;

    for (int i = 0; i < votes_received; i++)
    {
        int target_id = meeting_info.votes[i].target_id;
        if (target_id == VOTE_SKIP)
        {
            voting_result[MAX_PLAYERS] += 1;
            continue;
        }
        else if (target_id >= 0 && target_id < MAX_PLAYERS)
        {
            voting_result[target_id]++;
        }
    }

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (voting_result[i] > max_votes)
        {
            max_votes = voting_result[i];
            player_id = i;
        }
        else if (voting_result[i] == max_votes)
        {
            player_id = -1;
        }
    }

    if (max_votes <= voting_result[MAX_PLAYERS])
        player_id = -1;

    return player_id;
}

int resolve_voting(GameState *state, Meeting meeting_info, int vote_results[MAX_PLAYERS + 1])
{
    int vote_result = calculate_votes(meeting_info, vote_results);
    if (vote_result != -1)
    {
        state->players[vote_result].isAlive = 0;
    }
    return vote_result;
}
void handle_tcp_vote_connections(Server *s)
{
    TCPsocket incoming = NULL;
    while ((incoming = SDLNet_TCP_Accept(s->tcp_socket)) != NULL)
    {
        int stored = 0;
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (!s->tcpSockets[i])
            {
                s->tcpSockets[i] = incoming;
                s->voteBytesRead[i] = 0;
                SDLNet_TCP_AddSocket(s->tcpSocketSet, incoming);
                stored = 1;
                break;
            }
        }

        if (!stored)
        {
            printf("[SERVER] Rejected TCP vote connection: no free slot\n");
            SDLNet_TCP_Close(incoming);
        }
    }

    int ready = SDLNet_CheckSockets(s->tcpSocketSet, 0);
    if (ready <= 0)
        return;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        TCPsocket socket = s->tcpSockets[i];
        if (!socket || !SDLNet_SocketReady(socket))
            continue;

        /* Phase 1: read MessageType header */
        if (s->voteBytesRead[i] < (int)sizeof(MessageType))
        {
            int remaining = (int)sizeof(MessageType) - s->voteBytesRead[i];
            int received = SDLNet_TCP_Recv(socket,
                                           ((char *)&s->voteBuffers[i]) + s->voteBytesRead[i],
                                           remaining);
            if (received <= 0)
            {
                handle_leave_by_id(s, i);
                SDLNet_TCP_DelSocket(s->tcpSocketSet, socket);
                SDLNet_TCP_Close(socket);
                s->tcpSockets[i] = NULL;
                s->voteBytesRead[i] = 0;
                continue;
            }
            s->voteBytesRead[i] += received;
            if (s->voteBytesRead[i] < (int)sizeof(MessageType))
                continue;
        }

        /* Phase 2: determine full message size from type */
        MessageType type = s->voteBuffers[i].type;
        int target_size;
        if (type == MSG_TCP_HELLO)
            target_size = (int)sizeof(tcpHelloMessage);
        else if (type == MSG_VOTE_REQUEST)
            target_size = (int)sizeof(VoteRequest);
        else if (type == MSG_LEAVE)
            target_size = (int)sizeof(leaveMessage);
        else
        {
            s->voteBytesRead[i] = 0;
            continue;
        }

        /* Phase 3: read message body */
        if (s->voteBytesRead[i] < target_size)
        {
            int remaining = target_size - s->voteBytesRead[i];
            int received = SDLNet_TCP_Recv(socket,
                                           ((char *)&s->voteBuffers[i]) + s->voteBytesRead[i],
                                           remaining);
            if (received <= 0)
            {
                handle_leave_by_id(s, i);
                SDLNet_TCP_DelSocket(s->tcpSocketSet, socket);
                SDLNet_TCP_Close(socket);
                s->tcpSockets[i] = NULL;
                s->voteBytesRead[i] = 0;
                continue;
            }
            s->voteBytesRead[i] += received;
            if (s->voteBytesRead[i] < target_size)
                continue;
        }

        /* Dispatch complete message */
        if (type == MSG_TCP_HELLO)
        {
            tcpHelloMessage *hello = (tcpHelloMessage *)&s->voteBuffers[i];
            int pid = hello->player_id;
            if (pid != i && pid >= 0 && pid < MAX_PLAYERS && !s->tcpSockets[pid])
            {
                s->tcpSockets[pid] = socket;
                s->tcpSockets[i] = NULL;
                s->voteBytesRead[pid] = 0;
            }
        }
        else if (type == MSG_VOTE_REQUEST)
        {
            VoteRequest vote = s->voteBuffers[i];
            handle_tcp_vote(s, vote);
        }
        else if (type == MSG_LEAVE)
        {
            leaveMessage *leave = (leaveMessage *)&s->voteBuffers[i];
            handle_leave_by_id(s, leave->player_id);
        }
        s->voteBytesRead[i] = 0;
    }
}

void broadcast_vote_update(Server *s)
{
    VoteUpdateMsg msg = {0};
    msg.type = MSG_VOTE_UPDATE;
    msg.phase = s->state.phase;
    msg.meeting_reason = s->state.meeting_reason;
    msg.emergency_meeting_reported_id = s->state.emergency_meeting_reported_id;
    msg.voting_result = s->state.voting_result;

    for (int i = 0; i < MAX_PLAYERS + 1; i++)
        msg.voting_results[i] = s->state.voting_results[i];

    broadcast_tcp_msg(s->tcpSockets, &msg, sizeof(msg));
}
