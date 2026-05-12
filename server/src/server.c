#define SDL_MAIN_HANDLED
#include <SDL2/SDL_net.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "network.h"
#include "network_data.h"
#include "player_movement.h"
#include "imposter_ability.h"
#define PACKET_SIZE 1024

typedef struct {
    UDPsocket socket;
    UDPpacket *send_packet;
    UDPpacket *receive_packet;
    gameState state;
    Meeting meeting_info;
    IPaddress clientAddresses[MAX_PLAYERS];
    int clientUsed[MAX_PLAYERS];
    clientInput lastInput[MAX_PLAYERS];
    Uint64 state_start_time;
    Uint64 phase_time;
} Server;


static int init_server(UDPsocket *socket);
static int send_game_state(UDPsocket socket, UDPpacket *packet, IPaddress addr, gameState *state);
static int send_kill_msg(UDPsocket socket, UDPpacket *packet, IPaddress address, KillEventMsg *msg);
void cleanupServer(UDPsocket server_socket, UDPpacket *receive_packet, UDPpacket *send_packet);
int get_player_id_from_sender(IPaddress *clientAddresses, int *clientUsed, IPaddress sender);
int addToLobby(gameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr);
void spawn_players(gameState *state);
int countActivePlayers(gameState *state);
int designateImpostor(gameState *state);
static void shuffle_tasks(TaskType *arr, int n);
void start_new_round(gameState *state, Uint64 *state_start_time);
void check_win_condition(gameState *state);
void broadcastGameState(UDPsocket socket, UDPpacket *packet, gameState *state, IPaddress *clientAddresses, int *clientUsed);
void broadcast_Kill_msg(UDPsocket socket, UDPpacket *packet, KillEventMsg *msg, IPaddress *clientAddresses, int *clientUsed);
void broadcast_emergency_meeting_msg(UDPsocket socket, UDPpacket *packet, KillEventMsg *msg, IPaddress *clientAddresses, int *clientUsed);
int removeFromLobby(gameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr);
void apply_player_input(gameState *state, clientInput *input, float dt);
int can_cast_vote(Meeting meeting_info, int voter_id);
void cast_vote(Meeting *meeting_info, VoteRequest vote);
int calculate_votes(Meeting meeting_info, int voting_result[MAX_PLAYERS + 1]);
void resolve_voting(gameState *state, Meeting meeting_info, int vote_results[MAX_PLAYERS]);
void inititate_meeting_info(Meeting *meeting_info, gameState state);

void update_server_tick(Server *s);
void handle_join(Server *s, IPaddress sender);
void handle_leave(Server *s, IPaddress sender);
void handle_start_game(Server *s);
void handle_play_again(Server *s);
void handle_client_input(Server *s, IPaddress sender);
void handle_kill(Server *s, IPaddress sender);
void handle_emergency_meeting(Server *s, IPaddress sender);
void handle_body_found(Server *s, IPaddress sender);
void handle_task_complete(Server *s, IPaddress sender);
void handle_vote(Server *s, IPaddress sender);

int main(void)
{
    srand(time(NULL));

    Server server = {0};

    server.state.type = MSG_GAME_STATE;
    server.state.phase = GAME_LOBBY;

    for (int i = 0; i < MAX_PLAYERS; i++)
        server.lastInput[i].player_id = -1;

    if (!init_server(&server.socket))
        return 1;

    server.receive_packet = create_packet(PACKET_SIZE);
    if (!server.receive_packet)
    {
        cleanupServer(server.socket, NULL, NULL);
        return 1;
    }

    server.send_packet = create_packet(PACKET_SIZE);
    if (!server.send_packet)
    {
        cleanupServer(server.socket, server.receive_packet, NULL);
        return 1;
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    Uint64 lastBroadcast = SDL_GetPerformanceCounter();

    while (1)
    {
        if (SDLNet_UDP_Recv(server.socket, server.receive_packet))
        {
            MessageType type;

            if (!packet_has_size(server.receive_packet, sizeof(MessageType), "MessageType"))
                continue;
            memcpy(&type, server.receive_packet->data, sizeof(MessageType));
            IPaddress sender = server.receive_packet->address;

            switch (type)
            {
                case MSG_JOIN:                  handle_join(&server, sender);              break;
                case MSG_LEAVE:                 handle_leave(&server, sender);             break;
                case MSG_START_GAME:            handle_start_game(&server);                break;
                case MSG_PLAY_AGAIN:            handle_play_again(&server);                break;
                case MSG_CLIENT_INPUT:          handle_client_input(&server, sender);      break;
                case MSG_KILL_REQUEST:          handle_kill(&server, sender);              break;
                case MSG_EMERGENCY_MEETING:     handle_emergency_meeting(&server, sender); break;
                case MSG_BODY_FOUND:            handle_body_found(&server, sender);        break;
                case MSG_TASK_COMPLETE:         handle_task_complete(&server, sender);     break;
                case MSG_VOTE_REQUEST:          handle_vote(&server, sender);              break;
                case MSG_DEBUG_CREWMATES_WIN:   server.state.phase = GAME_CREWMATES_WIN;   break;
                case MSG_DEBUG_IMPOSTOR_WIN:    server.state.phase = GAME_IMPOSTOR_WIN;    break;
                default: break;
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float broadcastDt = (float)(now - lastBroadcast) / (float)SDL_GetPerformanceFrequency();

        if (broadcastDt >= SERVER_TICK_INTERVAL)
        {
            update_server_tick(&server);
            lastBroadcast = now;
        }
    }

    cleanupServer(server.socket, server.receive_packet, server.send_packet);
    return 0;
}

// ===================== SERVER TICK =====================

void update_server_tick(Server *s)
{
    if (s->state.phase == GAME_SHOW_ROLE)
    {
        if (SDL_GetTicks64() - s->state_start_time >= SHOW_ROLE_DURATION)
        {
            s->state.phase = GAME_RUNNING;
            printf("Game is now GAME_RUNNING\n");
        }
    }
    else if (s->state.phase == GAME_INFO_MEETING)
    {
        if (SDL_GetTicks64() - s->phase_time >= INFO_MEETING_DURATION)
        {
            s->state.phase = GAME_MEETING;
            inititate_meeting_info(&s->meeting_info, s->state);
            s->phase_time = SDL_GetTicks64();
            printf("INFORMATION OF MEETING ENDED\n");
        }
    }
    else if (s->state.phase == GAME_MEETING)
    {
        Uint32 elapsed = SDL_GetTicks64() - s->phase_time;
        Uint32 meeting_duration = MEETING_DURATION;
        s->state.meeting_time_remaining = (elapsed < meeting_duration)
            ? (int)(meeting_duration - elapsed)
            : 0;

        if (elapsed >= meeting_duration || s->meeting_info.votes_recieved == s->meeting_info.alive_players_count)
        {
            s->state.phase = SHOW_VOTE_RESULT;
            s->state.voting_result = calculate_votes(s->meeting_info, s->state.voting_results);
            printf("MEETING ENDED\n");
            s->phase_time = SDL_GetTicks64();
        }
    }
    else if (s->state.phase == SHOW_VOTE_RESULT)
    {
        if (SDL_GetTicks64() - s->phase_time >= VOTE_RESULT_DURATION)
        {
            resolve_voting(&s->state, s->meeting_info, s->state.voting_results);
            spawn_players(&s->state);
            s->state.phase = GAME_RUNNING;
            check_win_condition(&s->state);
        }
    }
    else if (s->state.phase == GAME_RUNNING)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            apply_player_input(&s->state, &s->lastInput[i], SERVER_TICK_INTERVAL);
            if (s->state.players[i].kill_cooldown_active)
            {
                s->state.players[i].kill_cooldown_active = update_kill_cooldown(s->state, i);
            }
            s->lastInput[i].player_id = -1;
            s->lastInput[i].up = 0;
            s->lastInput[i].down = 0;
            s->lastInput[i].left = 0;
            s->lastInput[i].right = 0;
            s->lastInput[i].current_frame = 0;
            s->lastInput[i].direction = DIR_DOWN;
        }
    }

    broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
}

// ===================== MESSAGE HANDLERS =====================

void handle_join(Server *s, IPaddress sender)
{
    int existing = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (existing < 0)
    {
        int newPlayer = addToLobby(&s->state, s->clientAddresses, s->clientUsed, sender);
        if (newPlayer >= 0)
        {
            printf("Player %d joined\n", newPlayer);
            broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
        }
    }
}

void handle_leave(Server *s, IPaddress sender)
{
    int removed = removeFromLobby(&s->state, s->clientAddresses, s->clientUsed, sender);
    if (removed >= 0)
    {
        printf("Player %d left\n", removed);
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_start_game(Server *s)
{
    if (s->state.phase == GAME_LOBBY)
    {
        start_new_round(&s->state, &s->state_start_time);
        printf("Game is now GAME_SHOW_ROLE\n");
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_play_again(Server *s)
{
    if (s->state.phase == GAME_CREWMATES_WIN || s->state.phase == GAME_IMPOSTOR_WIN)
    {
        start_new_round(&s->state, &s->state_start_time);
        printf("Play again: game is now GAME_SHOW_ROLE\n");
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_client_input(Server *s, IPaddress sender)
{
    if (s->state.phase != GAME_RUNNING)
        return;

    if (packet_has_size(s->receive_packet, sizeof(clientInput), "MSG_CLIENT_INPUT"))
    {
        clientInput input;
        memcpy(&input, s->receive_packet->data, sizeof(clientInput));
        int sender_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
        if (sender_id >= 0)
        {
            input.player_id = sender_id;
            s->lastInput[sender_id] = input;
        }
    }
}

void handle_kill(Server *s, IPaddress sender)
{
    if (!packet_has_size(s->receive_packet, sizeof(clientInput), "MSG_KILL_REQUEST"))
        return;

    int killer_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (killer_id < 0 || killer_id >= MAX_PLAYERS)
        return;

    int target_id = handle_kill_request(&s->state, killer_id);
    if (target_id != -1)
    {
        s->state.players[target_id].isAlive = 0;
        KillEventMsg msg = {0};
        msg.type = MSG_KILL_EVENT;
        msg.killer_id = killer_id;
        msg.victim_id = target_id;
        msg.x = s->state.players[killer_id].x;
        msg.y = s->state.players[killer_id].y;
        broadcast_Kill_msg(s->socket, s->send_packet, &msg, s->clientAddresses, s->clientUsed);
        check_win_condition(&s->state);
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_emergency_meeting(Server *s, IPaddress sender)
{
    if (!packet_has_size(s->receive_packet, sizeof(clientInput), "MSG_EMERGENCY_MEETING"))
        return;

    int local_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (local_id >= 0 && local_id < MAX_PLAYERS && s->state.players[local_id].isAlive && s->state.players[local_id].emergency_meeting == 1)
    {
        s->state.phase = GAME_INFO_MEETING;
        s->state.type = MSG_EMERGENCY_MEETING;
        s->state.players[local_id].emergency_meeting = 0;
        s->state.emergency_meeting_reported_id = local_id;
        printf("[SERVER] Accept: player %d started an emergency meeting.\n", local_id);
        s->phase_time = SDL_GetTicks64();
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_body_found(Server *s, IPaddress sender)
{
    if (!packet_has_size(s->receive_packet, sizeof(clientInput), "MSG_BODY_FOUND"))
        return;

    int local_id = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (local_id < 0 || local_id >= MAX_PLAYERS) 
        return;

    clientInput report;
    memcpy(&report, s->receive_packet->data, sizeof(report));

    Position dead_body = report.dead_body;
    int player_x = s->state.players[local_id].x;
    int player_y = s->state.players[local_id].y;

    if (s->state.players[local_id].isAlive && find_target_report_body(dead_body, player_x, player_y))
    {
        s->state.phase = GAME_INFO_MEETING;
        s->state.type = MSG_BODY_FOUND;
        s->state.emergency_meeting_reported_id = local_id;
        printf("[SERVER] Accept: player %d found a body.\n", local_id);
        s->phase_time = SDL_GetTicks64();
        broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
    }
}

void handle_task_complete(Server *s, IPaddress sender)
{
    if (!packet_has_size(s->receive_packet, sizeof(TaskCompleteMsg), "MSG_TASK_COMPLETE"))
        return;

    TaskCompleteMsg msg;
    memcpy(&msg, s->receive_packet->data, sizeof(msg));

    int pid = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    if (pid >= 0 && s->state.phase == GAME_RUNNING && s->state.players[pid].active && !s->state.players[pid].isImpostor)
    {
        int completed = s->state.players[pid].tasks_completed;
        if (completed < TASK_COUNT)
        {
            TaskType expected = s->state.players[pid].task_order[completed];
            if (msg.task_type == expected)
            {
                s->state.players[pid].tasks_completed++;
                s->state.total_tasks_completed++;
                check_win_condition(&s->state);

                int active_count = countActivePlayers(&s->state);
                int total_expected_tasks = TASK_COUNT * (active_count - 1);
                printf("\n=== TASK COMPLETE ===\n");
                printf("Player %d finished task %d/%d (TaskType %d)\n", pid, s->state.players[pid].tasks_completed, TASK_COUNT, msg.task_type);
                printf("Team progress: %d/%d\n", s->state.total_tasks_completed, total_expected_tasks);
                printf("=====================\n");
            }
            else
            {
                printf("[SERVER] Player %d sent wrong task type (got %d, expected %d)\n", pid, msg.task_type, expected);
            }
        }
    }
    broadcastGameState(s->socket, s->send_packet, &s->state, s->clientAddresses, s->clientUsed);
}

void handle_vote(Server *s, IPaddress sender)
{
    if (!packet_has_size(s->receive_packet, sizeof(VoteRequest), "VoteRequest"))
        return;

    VoteRequest vote;
    memcpy(&vote, s->receive_packet->data, sizeof(vote));
    int pid = get_player_id_from_sender(s->clientAddresses, s->clientUsed, sender);
    vote.voter_id = pid;
    if (can_cast_vote(s->meeting_info, pid) && s->meeting_info.votes_recieved < s->meeting_info.alive_players_count)
        cast_vote(&s->meeting_info, vote);
}

// ===================== NETWORK HELPERS =====================

static int init_server(UDPsocket *socket)
{
    return init_network_socket(socket, SERVER_PORT);
}

static int send_game_state(UDPsocket socket, UDPpacket *packet, IPaddress addr, gameState *state)
{
    return send_packet_data(socket, packet, addr, state, sizeof(*state));
}

static int send_kill_msg(UDPsocket socket, UDPpacket *packet, IPaddress address, KillEventMsg *msg)
{
    return send_packet_data(socket, packet, address, msg, sizeof(*msg));
}

void cleanupServer(UDPsocket server_socket, UDPpacket *receive_packet, UDPpacket *send_packet)
{
    if (receive_packet)
        SDLNet_FreePacket(receive_packet);
    if (send_packet)
        SDLNet_FreePacket(send_packet);
    if (server_socket)
        SDLNet_UDP_Close(server_socket);
    SDLNet_Quit();
}

void broadcastGameState(UDPsocket socket, UDPpacket *packet, gameState *state, IPaddress *clientAddresses, int *clientUsed)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i])
        {
            state->local_player_id = i;
            if (!send_game_state(socket, packet, clientAddresses[i], state))
            {
                printf("Failed to send game state to player %d\n", i);
            }
        }
    }
}

void broadcast_Kill_msg(UDPsocket socket, UDPpacket *packet, KillEventMsg *msg, IPaddress *clientAddresses, int *clientUsed)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i])
        {
            if (!send_kill_msg(socket, packet, clientAddresses[i], msg))
            {
                printf("Failed to send kill msg to player %d\n", i);
            }
        }
    }
}

void broadcast_emergency_meeting_msg(UDPsocket socket, UDPpacket *packet, KillEventMsg *msg, IPaddress *clientAddresses, int *clientUsed)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i])
        {
            if (!send_kill_msg(socket, packet, clientAddresses[i], msg))
            {
                printf("Failed to send emergency meeting msg to player %d\n", i);
            }
        }
    }
}

// ===================== LOBBY & PLAYERS =====================

int get_player_id_from_sender(IPaddress *clientAddresses, int *clientUsed, IPaddress sender)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (clientUsed[i] &&
            clientAddresses[i].host == sender.host &&
            clientAddresses[i].port == sender.port)
        {
            return i;
        }
    }
    return -1;
}

int addToLobby(gameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr)
{
    float spawnX[MAX_PLAYERS] = {1290, 1150, 1420, 1000, 1290, 1150};
    float spawnY[MAX_PLAYERS] = {665, 665, 850, 850, 1000, 1000};

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!clientUsed[i])
        {
            clientUsed[i] = 1;
            clientAddresses[i] = addr;

            state->players[i].active = 1;
            state->players[i].isAlive = 1;
            state->players[i].player_id = i;
            state->players[i].x = spawnX[i];
            state->players[i].y = spawnY[i];
            state->players[i].current_frame = 2;
            state->players[i].direction = DIR_DOWN;
            state->players[i].isImpostor = 0;
            state->players[i].kill_cooldown_start = 0;
            state->players[i].kill_cooldown_active = false;
            state->players[i].emergency_meeting = 1;
            state->emergency_meeting_reported_id = -1;
            return i;
        }
    }
    return -1;
}

int removeFromLobby(gameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr)
{
    int player = get_player_id_from_sender(clientAddresses, clientUsed, addr);
    if (player >= 0)
    {
        clientUsed[player] = 0;
        state->players[player].active = 0;
        state->players[player].player_id = -1;
        return player;
    }
    return -1;
}

void spawn_players(gameState *state)
{
    float spawnX[MAX_PLAYERS] = {1290, 1150, 1420, 1000, 1290, 1150};
    float spawnY[MAX_PLAYERS] = {665, 665, 850, 850, 1000, 1000};

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        state->players[i].x = spawnX[i];
        state->players[i].y = spawnY[i];
        state->players[i].current_frame = 2;
        state->players[i].direction = DIR_DOWN;
        state->players[i].kill_cooldown_start = 0;
        state->players[i].kill_cooldown_active = false;
    }
}

int countActivePlayers(gameState *state)
{
    int active_players = 0;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (state->players[i].active)
            active_players++;
    }

    return active_players;
}

// ===================== GAME LOGIC =====================

int designateImpostor(gameState *state)
{
    int active_player_count = countActivePlayers(state);
    int chosen_active_player = 0;
    int active_player_index = 0;

    chosen_active_player = rand() % active_player_count;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!state->players[i].active)
            continue;

        if (active_player_index == chosen_active_player)
        {
            state->players[i].isImpostor = 1;
            return chosen_active_player;
        }

        active_player_index++;
    }
    return -1;
}

static void shuffle_tasks(TaskType *arr, int n)
{
    for (int i = n - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        TaskType tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

void start_new_round(gameState *state, Uint64 *state_start_time)
{
    TaskType all_tasks[TASK_COUNT] = {
        TASK_TIMER,
        TASK_CLICK,
        TASK_LETTER,
        TASK_REFLEX,
        TASK_LOGICAL_ORDER,
        TASK_MEMORY,
        TASK_HOLD,
        TASK_ALTERNATE};

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        state->players[i].isImpostor = 0;

        if (!state->players[i].active)
            continue;

        state->players[i].isAlive = 1;
        state->players[i].emergency_meeting = 1;
        state->players[i].tasks_completed = 0;
        memcpy(state->players[i].task_order, all_tasks, sizeof(all_tasks));
        shuffle_tasks(state->players[i].task_order, TASK_COUNT);
    }

    spawn_players(state);

    int active_chosen_player = designateImpostor(state);
    printf("Player %d is impostor\n", active_chosen_player);

    printf("\n=== TASK ORDER ASSIGNMENT ===\n");
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!state->players[i].active)
            continue;

        printf("Player %d: ", i);
        for (int j = 0; j < TASK_COUNT; j++)
        {
            printf("%d ", state->players[i].task_order[j]);
        }
        printf("\n");
    }
    printf("=============================\n");

    state->emergency_meeting_reported_id = -1;
    state->total_tasks_completed = 0;
    state->phase = GAME_SHOW_ROLE;
    *state_start_time = SDL_GetTicks64();
}

void check_win_condition(gameState *state)
{
    int alive_impostor = 0;
    int alive_crewmates = 0;
    int active_crewmates = 0;
    int completed_tasks = 0;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!state->players[i].active)
            continue;

        if (state->players[i].isImpostor)
        {
            if (state->players[i].isAlive)
                alive_impostor++;
            continue;
        }

        active_crewmates++;
        completed_tasks += state->players[i].tasks_completed;

        if (state->players[i].isAlive)
            alive_crewmates++;
    }

    if (alive_impostor == 0)
        state->phase = GAME_CREWMATES_WIN;
    else if (alive_impostor >= alive_crewmates)
        state->phase = GAME_IMPOSTOR_WIN;
    else if (active_crewmates > 0 && completed_tasks >= active_crewmates * TASK_COUNT)
        state->phase = GAME_CREWMATES_WIN;
}

// ===================== INPUT & MOVEMENT =====================

void apply_player_input(gameState *state, clientInput *input, float dt)
{
    int id = input->player_id;
    if (id < 0 || id >= MAX_PLAYERS)
        return;
    if (!state->players[id].active)
        return;

    state->players[id].current_frame = input->current_frame;
    state->players[id].direction = input->direction;

    apply_movement(&state->players[id].x, &state->players[id].y, *input, dt);
}

// ===================== VOTING =====================

int can_cast_vote(Meeting meeting_info, int voter_id)
{
    if (voter_id < 0 || voter_id >= MAX_PLAYERS)
        return 0;

    if (meeting_info.has_voted[voter_id])
        return 0;

    return 1;
}

void cast_vote(Meeting *meeting_info, VoteRequest vote)
{
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
        voting_result[target_id]++;
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

void resolve_voting(gameState *state, Meeting meeting_info, int vote_results[MAX_PLAYERS])
{
    int vote_result = calculate_votes(meeting_info, vote_results);
    if (vote_result != -1)
    {
        state->players[vote_result].isAlive = 0;
    }
}

void inititate_meeting_info(Meeting *meeting_info, gameState state)
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
        if (!state.players[i].isAlive || !state.players[i].active)
            continue;

        meeting_info->alive_players_id[meeting_info->alive_players_count] = i;
        meeting_info->alive_players_count++;
    }
}