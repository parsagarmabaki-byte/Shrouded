#ifndef TASK_H
#define TASK_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct Task Task;

typedef enum {
    TASK_NONE,
    TASK_TIMER,
    TASK_CLICK,
    TASK_TYPE,
    TASK_REFLEX,
    TASK_LOGICAL_ORDER
} TaskType;

// constructor / destructor
Task* create_task(SDL_Renderer *renderer);
void destroy_task(Task *task);

// behavior
void start_timer_task(Task *task, SDL_Renderer *renderer, float duration);
void start_click_task(Task *task, SDL_Renderer *renderer, int target);
void start_type_task(Task *task, SDL_Renderer *renderer);
void start_reflex_task(Task *task, SDL_Renderer *renderer);

void update_task(Task *task, float dt);
void render_task(SDL_Renderer *renderer, Task *task);
void cancel_task(Task *task);
void cleanup_task(Task *task);
void complete_task(Task *task);

// getters (important for ADT)
bool task_is_active(Task *task);
int task_get_points(Task *task);

// handle input events for tasks
void task_handle_key(Task *task, SDL_Keycode key);
void task_handle_click(Task *task);

#endif