#ifndef TASK_RENDER_H
#define TASK_RENDER_H

#include "task_internal.h"


void render_task(SDL_Renderer *renderer, Task *task, int screen_width, int screen_height);
void render_timer_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height);
void render_click_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height);
void render_letter_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height);
void render_reflex_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height);
void render_logical_order_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height);
void render_memory_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height);
void render_hold_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height);
void render_alternate_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height);

#endif