#include "server_round.h"
#include "server_lobby.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

int designateImpostor(gameState *state)
{
    int active_player_count = countActivePlayers(state);
    int chosen_active_player = 0;
    int active_player_index = 0;

    if (active_player_count <= 0)
        return -1;

    chosen_active_player = rand() % active_player_count;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!state->players[i].active)
            continue;

        if (active_player_index == chosen_active_player)
        {
            state->players[i].isImpostor = 1;
            return i;
        }

        active_player_index++;
    }
    return -1;
}

void start_new_round(gameState *state, Uint64 *state_start_time, int *killer_id)
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

    *killer_id = designateImpostor(state);
    if (*killer_id >= 0)
        printf("Player %d is impostor\n", *killer_id);

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
    state->meeting_reason = MEETING_NONE;
    state->voting_result = -1;
    for (int i = 0; i < MAX_PLAYERS + 1; i++)
        state->voting_results[i] = 0;
    state->total_tasks_completed = 0;
    state->phase = GAME_SHOW_ROLE;
    *state_start_time = SDL_GetTicks64();
}
