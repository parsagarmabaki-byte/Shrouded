#ifndef TASK_H
#define TASK_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef enum {
    TASK_NONE,
    TASK_TIMER
} TaskType;

typedef struct {
    TaskType type;
    bool active;
    int points;

    // generic timer (only used if TASK_TIMER)
    float timer;

} Task;

// API
void init_task(Task *task);
void start_timer_task(Task *task, float duration);
void update_task(Task *task, float dt);
void render_task(SDL_Renderer *renderer, Task *task);
void cancel_task(Task *task);

#endif