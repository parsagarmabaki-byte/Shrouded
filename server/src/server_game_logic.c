#include "server_game_logic.h"
#include "player_movement.h"

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
