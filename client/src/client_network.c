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

    client->recievepacket = create_packet(1024);
    if (!client->recievepacket)
    {
        snprintf(error_message, error_size, "Could not allocate receive packet");
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


static int send_packet(UDPsocket socket, IPaddress server_addr, const void *data, size_t size)
{
    UDPpacket *packet = create_packet(512);
    if (!packet)
        return 0;

    if (!send_packet_data(socket, packet,server_addr, data, size))
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
void send_input(Client *client, gameState *state, Player *player)
{
    clientInput input = read_input(false);
    input.type = MSG_CLIENT_INPUT;
    input.player_id = state->local_player_id;
    input.current_frame = player->current_frame;
    input.direction = player->direction;

    send_packet(client->socket, client->serverAddr, &input, sizeof(clientInput));
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

void request_report_body(Client *client, gameState *state, KillAnimation dead_body, int target_id)
{
    clientInput input = {0};
    input.type = MSG_BODY_FOUND;
    input.player_id = state->local_player_id;
    input.target_id = target_id;
    input.dead_body.x = dead_body.x;
    input.dead_body.y = dead_body.y;
    // printf("[CLIENT] Sending MSG_BODY_FOUND\n");
    // printf("[CLIENT] local_player_id=%d target_id=%d\n",
    //        input.player_id, input.target_id);
    // printf("[CLIENT] dead_body=(%d, %d)\n",
    //        input.dead_body.x, input.dead_body.y);
    send_packet(client->socket, client->serverAddr, &input, sizeof(clientInput));
}

void request_emergency_meeting(Client *client, gameState *state, int local_id)
{
    clientInput request = {0};
    request.type = MSG_EMERGENCY_MEETING;
    request.emergency_meeting_left = state->players[local_id].emergency_meeting;
    request.isAlive = state->players[local_id].isAlive;
    request.player_id = local_id;
    send_packet(client->socket, client->serverAddr, &request, sizeof(clientInput));
}

void send_vote(Client *client, int targeted_banner)
{
    VoteRequest vote;
    vote.type = MSG_VOTE_REQUEST;
    vote.target_id = targeted_banner;
    send_packet(client->socket, client->serverAddr, &vote,sizeof(VoteRequest));
}

void collect_packets(Client *client, gameState *state, KillAnimation *bodies, AudioAssets *audio)
{
    while (SDLNet_UDP_Recv(client->socket, client->recievepacket))
    {
        if (!packet_has_size(client->recievepacket, sizeof(MessageType), "MessageType"))
        {
            continue;
        }

        MessageType type;
        memcpy(&type, client->recievepacket->data, sizeof(type));

        if (type == MSG_GAME_STATE || type == MSG_EMERGENCY_MEETING || type == MSG_BODY_FOUND)
        {
            if (packet_has_size(client->recievepacket, sizeof(gameState), "MSG_GAME_STATE"))
            {
                gamePhase previous_phase = state->phase;
                memcpy(state, client->recievepacket->data, sizeof(gameState));
                if (state->phase == GAME_INFO_MEETING && previous_phase != GAME_INFO_MEETING)
                {
                    play_meeting_horn(audio);
                }
            }
        }
        else if (type == MSG_KILL_EVENT)
        {
            KillEventMsg msg;
            if (packet_has_size(client->recievepacket, sizeof(KillEventMsg), "MSG_KILL_EVENT"))
            {
                memcpy(&msg, client->recievepacket->data, sizeof(KillEventMsg));
                state->players[msg.victim_id].isAlive = 0;
                if (bodies)
                {
                    start_kill_animation(&bodies[msg.victim_id], msg.killer_id, msg.victim_id,
                                         state->players[msg.victim_id].x,
                                         state->players[msg.victim_id].y);
                    printf("\nCLIENT %d START BODY victim=%d active=%d x=%.1f y=%.1f\n",
                           state->local_player_id,
                           msg.victim_id,
                           bodies[msg.victim_id].active,
                           bodies[msg.victim_id].x,
                           bodies[msg.victim_id].y);
                }
                if (state->local_player_id == msg.killer_id || state->local_player_id == msg.victim_id)
                {
                    play_kill_knife(audio);
                    play_dramatic_kill(audio);
                }
            }
        }
    }
}
