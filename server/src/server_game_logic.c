#include "server_game_logic.h"
#include "player_movement.h"
#include "server_broadcast.h"

void check_win_condition(GameState *state)
{
    if (state->phase == GAME_KILLER_WIN || state->phase == GAME_INNOCENTS_WIN)
        return;
    int alive_killer = 0;
    int alive_innocents = 0;
    int active_innocents = 0;
    int completed_tasks = 0;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!state->players[i].active)
            continue;

        if (state->players[i].isKiller)
        {
            if (state->players[i].isAlive)
                alive_killer++;
            continue;
        }

        active_innocents++;
        completed_tasks += state->players[i].tasks_completed;

        if (state->players[i].isAlive)
            alive_innocents++;
    }
    if (alive_killer == 0 || alive_killer >= alive_innocents || (active_innocents > 0 && completed_tasks >= active_innocents * TASK_COUNT))
    {
        if (alive_killer == 0)
            state->phase = GAME_INNOCENTS_WIN;
        else if (alive_killer >= alive_innocents)
            state->phase = GAME_KILLER_WIN;
        else if (active_innocents > 0 && completed_tasks >= active_innocents * TASK_COUNT)
            state->phase = GAME_INNOCENTS_WIN;
    }
}

void apply_player_input(GameState *state, InputMsg *input, float dt)
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
