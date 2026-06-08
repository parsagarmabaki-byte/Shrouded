#include "server_meeting.h"
#include "server_broadcast.h"
#include <stdio.h>

void inititate_meeting_info(Meeting *meeting_info, gameState *state)
{
    meeting_info->alive_players_count = 0;
    meeting_info->votes_recieved = 0;

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
    if (meeting_info->votes_recieved < 0 || meeting_info->votes_recieved >= MAX_PLAYERS)
        return;
    if (vote.voter_id < 0 || vote.voter_id >= MAX_PLAYERS)
        return;
    if (vote.target_id != VOTE_SKIP && (vote.target_id < 0 || vote.target_id >= MAX_PLAYERS))
        return;

    int index = meeting_info->votes_recieved;
    meeting_info->votes[index] = vote;
    meeting_info->has_voted[vote.voter_id] = 1;
    meeting_info->votes_recieved++;
    printf("\nVote accepted: voter=%d target=%d votes=%d/%d\n",
           vote.voter_id,
           vote.target_id,
           meeting_info->votes_recieved,
           meeting_info->alive_players_count);
}

int calculate_votes(Meeting meeting_info, int voting_result[MAX_PLAYERS + 1])
{
    int votes_recieved = meeting_info.votes_recieved;
    int max_votes = 0;
    int player_id = -1;

    for (int i = 0; i < MAX_PLAYERS + 1; i++)
        voting_result[i] = 0;

    for (int i = 0; i < votes_recieved; i++)
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

    printf("\nVOTING RESULT IS TO KICK OUT player id %d\n", player_id);
    return player_id;
}

int resolve_voting(gameState *state, Meeting meeting_info, int vote_results[MAX_PLAYERS + 1])
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
            if (!s->voteSockets[i])
            {
                s->voteSockets[i] = incoming;
                s->voteBytesRead[i] = 0;
                SDLNet_TCP_AddSocket(s->voteSocketSet, incoming);
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

    int ready = SDLNet_CheckSockets(s->voteSocketSet, 0);
    if (ready <= 0)
        return;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        TCPsocket socket = s->voteSockets[i];
        if (!socket || !SDLNet_SocketReady(socket))
            continue;

        int remaining = (int)sizeof(VoteRequest) - s->voteBytesRead[i];
        int received = SDLNet_TCP_Recv(socket,
                                       ((char *)&s->voteBuffers[i]) + s->voteBytesRead[i],
                                       remaining);
        if (received <= 0)
        {
            SDLNet_TCP_DelSocket(s->voteSocketSet, socket);
            SDLNet_TCP_Close(socket);
            s->voteSockets[i] = NULL;
            s->voteBytesRead[i] = 0;
            continue;
        }

        s->voteBytesRead[i] += received;
        if (s->voteBytesRead[i] == (int)sizeof(VoteRequest))
        {
            VoteRequest vote = s->voteBuffers[i];
            s->voteBytesRead[i] = 0;
            if (vote.type == MSG_VOTE_REQUEST)
                handle_tcp_vote(s, vote);
        }
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

    broadcast_tcp_msg(s->voteSockets, &msg, sizeof(msg));
}
