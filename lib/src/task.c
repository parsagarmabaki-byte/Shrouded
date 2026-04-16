#include "task.h"
#include <stdio.h>

void init_task(Task *task)
{
    task->type = TASK_NONE;
    task->active = false;
    task->timer = 0.0f;
    task->points = 0;
}

void start_timer_task(Task *task, float duration)
{
    task->type = TASK_TIMER;
    task->active = true;
    task->timer = duration;
}

void update_task(Task *task, float dt)
{
    if (!task->active) return;

    switch (task->type)
    {
        case TASK_TIMER:
            task->timer -= dt;
            if (task->timer <= 0.0f)
            {
                task->active = false;
                task->type = TASK_NONE;
                task->points +=1;
            }
            break;

        default:
            break;
    }
}

void cancel_task(Task *task)
{
    if (!task->active) return;

    task->active = false;
    task->type = TASK_NONE;
}

void render_task(SDL_Renderer *renderer, Task *task)
{
    if (!task->active) return;

    switch (task->type)
    {
        case TASK_TIMER:
        {
            SDL_Rect panel = {600, 300, 700, 400};

            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &panel);

            // (later: draw timer text here)

            break;
        }

        default:
            break;
    }
}