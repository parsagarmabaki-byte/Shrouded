#ifndef TASK_INIT_H
#define TASK_INIT_H

#include "task.h"

void start_timer_task(Task *task, SDL_Renderer *renderer, float duration);
void start_click_task(Task *task, SDL_Renderer *renderer, int target);
void start_letter_task(Task *task, SDL_Renderer *renderer);
void start_reflex_task(Task *task, SDL_Renderer *renderer);
void start_logical_order_task(Task *task, SDL_Renderer *renderer);
void start_memory_task(Task *task, SDL_Renderer *renderer);
void start_hold_task(Task *task, SDL_Renderer *renderer, float duration);
void start_alternate_task(Task *task, SDL_Renderer *renderer, int target);

#endif