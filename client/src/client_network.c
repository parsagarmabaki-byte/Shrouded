#include <SDL2/SDL_net.h>
#include <stdio.h>
#include "network_data.h"
#include "game.h"


static int send_client_input_packet(UDPsocket socket, IPaddress server_addr, clientInput *input)
{
    UDPpacket *packet = create_packet(512);
    if (!packet)
    {
        return 0;
    }

    if (!send_packet_data(socket, packet, server_addr, input, sizeof(*input)))
    {
        SDLNet_FreePacket(packet);
        return 0;
    }

    SDLNet_FreePacket(packet);
    return 1;
}

int send_leave_message(UDPsocket socket, IPaddress server_addr)
{
    leaveMessage leave = {0};
    leave.type = MSG_LEAVE;

    UDPpacket *packet = create_packet(512);
    if (!packet)
    {
        return 0;
    }

    if (!send_packet_data(socket, packet, server_addr, &leave, sizeof(leave)))
    {
        SDLNet_FreePacket(packet);
        return 0;
    }

    SDLNet_FreePacket(packet);
    return 1;
}
void send_input(Client *client, gameState *state, Player *player)
{
    clientInput input = read_input(false);
    input.type = MSG_CLIENT_INPUT;
    input.player_id = state->local_player_id;
    input.current_frame = player->current_frame;
    input.direction = player->direction;

    send_client_input_packet(client->socket, client->serverAddr, &input);
}
void request_kill(Client *client, gameState *state)
{
    clientInput input = {0};
    input.type = MSG_KILL_REQUEST;
    input.player_id = state->local_player_id;
    send_client_input_packet(client->socket, client->serverAddr, &input);
}

void collect_packets(Client *client, gameState *state, KillAnimation *bodies)
{
    while (SDLNet_UDP_Recv(client->socket, client->recievepacket))
    {
        uint8_t type = client->recievepacket->data[0];

        if (type == MSG_GAME_STATE)
        {
            memcpy(state, client->recievepacket->data, sizeof(gameState));
        }
        else if (type == MSG_KILL_EVENT)
        {
            KillEventMsg msg;
            memcpy(&msg, client->recievepacket->data, sizeof(KillEventMsg));

            printf("Kill received: killer=%d victim=%d\n",
                   msg.killer_id, msg.victim_id);

            // start_kill_animation(state, msg.killer_id, msg.victim_id, msg.x, msg.y);
            start_kill_animation(&bodies[msg.victim_id], msg.killer_id, msg.victim_id,
                     state->players[msg.victim_id].x,
                     state->players[msg.victim_id].y);
        }
    }
}