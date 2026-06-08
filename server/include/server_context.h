#ifndef SERVER_CONTEXT_H
#define SERVER_CONTEXT_H

#include <SDL2/SDL_net.h>
#include "network_data.h"

typedef struct {
    UDPsocket socket;
    TCPsocket tcp_socket;
    TCPsocket voteSockets[MAX_PLAYERS];
    VoteRequest voteBuffers[MAX_PLAYERS];
    int voteBytesRead[MAX_PLAYERS];
    SDLNet_SocketSet voteSocketSet;
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
    Uint64 kill_cooldown_start;
    int last_timer_half_second;
    int killer_id;
} Server;

#endif
