#ifndef SERVER_CONTEXT_H
#define SERVER_CONTEXT_H

#include <SDL2/SDL_net.h>
#include "network_data.h"

typedef struct {
    UDPsocket socket;
    UDPpacket *send_packet;
    UDPpacket *receive_packet;
    gameState state;
    Meeting meeting_info;
    IPaddress clientAddresses[MAX_PLAYERS];
    int clientUsed[MAX_PLAYERS];
    InputMsg lastInput[MAX_PLAYERS];
    Position deadBodies[MAX_PLAYERS];
    int deadBodyActive[MAX_PLAYERS];
    Uint64 state_start_time;
    Uint64 phase_time;
} Server;

#endif
