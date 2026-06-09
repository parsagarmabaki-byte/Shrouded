#ifndef NETWORK_H
#define NETWORK_H

#include <SDL2/SDL_net.h>

int init_network_socket(UDPsocket *socket, Uint16 port);
int init_tcp_server_socket(TCPsocket *socket, Uint16 port);
UDPpacket *create_packet(int size);
int send_packet_data(UDPsocket socket, UDPpacket *packet, IPaddress address, const void *data, int size);
int send_tcp_data(TCPsocket socket, const void *data, int size);
int packet_has_size(UDPpacket *packet, int expectedsize, const char *label);
// void cast_vote(Meeting *meeting_info, VoteRequest vote);
// void initiate_meeting_info(Meeting *meeting_info, GameState state);
// int can_cast_vote(Meeting meeting_info, int voter_id);
// int calculate_votes(Meeting meeting_info, int voting_result[MAX_PLAYERS]);

#endif
