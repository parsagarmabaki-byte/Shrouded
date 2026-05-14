#ifndef SERVER_MEETING_H
#define SERVER_MEETING_H

#include "network_data.h"

void inititate_meeting_info(Meeting *meeting_info, gameState state);
int can_cast_vote(Meeting meeting_info, int voter_id);
void cast_vote(Meeting *meeting_info, VoteRequest vote);
int calculate_votes(Meeting meeting_info, int voting_result[MAX_PLAYERS + 1]);
void resolve_voting(gameState *state, Meeting meeting_info, int vote_results[MAX_PLAYERS]);

#endif
