#ifndef SERVER_MEETING_H
#define SERVER_MEETING_H

#include "network_data.h"
#include "server_context.h"

void initiate_meeting_info(Meeting *meeting_info, GameState *state);
int can_cast_vote(Meeting meeting_info, int voter_id);
void cast_vote(Meeting *meeting_info, VoteRequest vote);
int calculate_votes(Meeting meeting_info, int voting_result[MAX_PLAYERS + 1]);
int resolve_voting(GameState *state, Meeting meeting_info, int vote_results[MAX_PLAYERS + 1]);
void handle_tcp_vote_connections(Server *s);
void handle_tcp_vote(Server *s, VoteRequest vote);
#endif
