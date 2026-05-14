#include "task.h"
#include "task_render.h"
#include "task_internal.h"
#include "task_init.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool task_active_check(Task *task) // call these 4 getter functions outside task.c to access struct variables
{
    return task->active;
}

TaskType task_get_current_type(Task *task)
{
    return task->type;
}

TaskType task_get_last_type(Task *task)
{
    return task->last_completed_type;
}

TaskStatus task_get_status(Task *task)
{
    return task->status;
}

Task *create_task(SDL_Renderer *renderer)
{
    Task *task = malloc(sizeof(Task));
    if (!task)
        return NULL;

    // Zero-initialize the entire struct to prevent garbage values
    memset(task, 0, sizeof(Task));

    task->type = TASK_NONE;
    task->active = false;
    task->renderer = renderer;

    // Initialize Text objects
    task->global_text = text_create(renderer, "assets/fonts/BebasNeue-Regular.ttf", 32);
    task->task_text = text_create(renderer, "assets/fonts/BebasNeue-Regular.ttf", 32);
    task->dynamic_text = text_create(renderer, "assets/fonts/BebasNeue-Regular.ttf", 32);

    if (task->global_text)
        text_set(task->global_text, "PRESS Q TO ABANDON ASSIGNMENT", WHITE);
        
    return task;
}

void end_task(Task *task, TaskStatus status)
{
    // Save the type so other systems can see even after we clear it.
    task->last_completed_type = task->type;
    task->active = false;
    task->type = TASK_NONE;
    task->status = status; // Track HOW it ended
    cleanup_task(task);
}

void cleanup_task(Task *task) // cleans non specific things, used before starting a new task to free old textures and reset variables
{
    if (task->task_image)
    {
        SDL_DestroyTexture(task->task_image);
        task->task_image = NULL;
    }

    for (int i = 0; i < 5; i++)
    {
        if (task->number_texts[i])
        {
            text_destroy(task->number_texts[i]);
            task->number_texts[i] = NULL;
        }
    }
}

void destroy_task(Task *task) // cleans everything, used at the end of the game
{
    if (!task)
        return;

    cleanup_task(task);

    if (task->global_text)
    {
        text_destroy(task->global_text);
        task->global_text = NULL;
    }

    if (task->dynamic_text)
    {
        text_destroy(task->dynamic_text);
        task->dynamic_text = NULL;
    }

    if (task->task_text)
    {
        text_destroy(task->task_text);
        task->task_text = NULL;
    }

    free(task);
}

void update_task(Task *task, float dt) // updates task logic every frame
{
    if (!task->active)
        return;

    switch (task->type)
    {
    case TASK_TIMER:
        update_timer_task(task, dt);
        break;
    case TASK_CLICK:
        update_click_task(task);
        break;
    case TASK_LETTER:
        update_letter_task(task);
        break;
    case TASK_REFLEX:
        update_reflex_task(task, dt);
        break;
    case TASK_MEMORY:
        update_memory_task(task, dt);
        break;
    case TASK_HOLD:
        update_hold_task(task, dt);
        break;
    case TASK_ALTERNATE:
        update_alternate_task(task);
        break;
    case TASK_LOGICAL_ORDER:
        // No frame based logic needed
        break;
    default:
        break;
    }
}

void update_timer_task(Task *task, float dt)
{
    if (!task->active || task->type != TASK_TIMER)
        return;

    task->timer -= dt;
    if (task->timer <= 0.0f)
    {
        end_task(task, TASK_STATUS_COMPLETED);
    }
}

void update_click_task(Task *task)
{
    if (!task->active || task->type != TASK_CLICK)
        return;

    if (task->click_count >= task->click_target)
    {
        end_task(task, TASK_STATUS_COMPLETED);
    }
}

void update_letter_task(Task *task)
{
    if (!task->active || task->type != TASK_LETTER)
        return;

    if (task->current_index >= task->length)
    {
        end_task(task, TASK_STATUS_COMPLETED);
    }
}

void update_reflex_task(Task *task, float dt)
{
    if (!task->active || task->type != TASK_REFLEX)
        return;

    // move cursor
    task->cursor_pos += task->direction * task->cursor_speed * dt;

    // bounce at edges
    if (task->cursor_pos >= 1.0f)
    {
        task->cursor_pos = 1.0f;
        task->direction = -1;
    }
    else if (task->cursor_pos <= 0.0f)
    {
        task->cursor_pos = 0.0f;
        task->direction = 1;
    }
}

void update_memory_task(Task *task, float dt)
{
    if (!task->active || task->type != TASK_MEMORY)
        return;

    if (task->start_delay > 0.0f)
    {
        task->start_delay -= dt;
        return;
    }
    if (task->showing_sequence)
    {
        task->flash_timer -= dt;

        while (task->flash_timer <= 0.0f && task->showing_sequence)
        {
            if (task->flash_visible)
            {
                // go invisible
                task->flash_visible = false;
                task->flash_timer += task->flash_interval * 0.5f;
            }
            else
            {
                // next arrow
                task->flash_visible = true;
                task->flash_index++;

                if (task->flash_index >= task->sequence_length)
                {
                    task->showing_sequence = false;
                    task->input_index = 0;
                    break;
                }

                task->flash_timer += task->flash_interval;
            }
        }
    }
}

void update_hold_task(Task *task, float dt)
{
    if (!task->active || task->type != TASK_HOLD)
        return;

        if (task->hold_key_down)
                {
                    task->hold_timer += dt;
                    if (task->hold_timer >= task->hold_duration)
                    {
                        end_task(task, TASK_STATUS_COMPLETED);
                    }
                }
                else
                {
                    // Sjunker tillbaka om man inte håller
                    task->hold_timer -= dt * 0.5f;
                    if (task->hold_timer < 0.0f)
                        task->hold_timer = 0.0f;
                }
}

void update_alternate_task(Task *task)
{
    if (!task->active || task->type != TASK_ALTERNATE)
        return;

    if (task->alternate_count >= task->alternate_target)
    {
        end_task(task, TASK_STATUS_COMPLETED);
    }
}

// handle input events for tasks
void task_handle_key(Task *task, SDL_Keycode key)
{
    if (!task->active)
        return;

    if (task->type == TASK_REFLEX && key == SDLK_SPACE)
    {
        handle_reflex_key(task, key);
        return;
    }

    if (task->type == TASK_LETTER)
    {
        handle_letter_key(task, key);
        return;
    }

    if (task->type == TASK_MEMORY && !task->showing_sequence)
    {
        handle_memory_key(task, key);
        return;
    }
    if (task->type == TASK_HOLD)
    {
        handle_hold_key(task, key);
        return;
    }

    if (task->type == TASK_ALTERNATE)
    {
        handle_alternate_key(task, key);
        return;
    }
}

static void handle_reflex_key(Task *task, SDL_Keycode key)
{
    if (task->cursor_pos >= task->success_min &&
        task->cursor_pos <= task->success_max)
    {
        // success
        (task->success_count)++;

        // shrink zone
        task->current_zone_width *= 0.8f;
        if (task->current_zone_width < 0.05f)
            task->current_zone_width = 0.05f;

        float center = (task->success_min + task->success_max) / 2.0f;
        task->success_min = center - task->current_zone_width / 2.0f;
        task->success_max = center + task->current_zone_width / 2.0f;

        if (task->success_min < 0.0f)
            task->success_min = 0.0f;
        if (task->success_max > 1.0f)
            task->success_max = 1.0f;

        if (task->success_count >= task->success_target)
        {
            end_task(task, TASK_STATUS_COMPLETED);
        }
    }
    else
    {
        // failure reset
        task->success_count = 0;
        task->cursor_pos = 0.0f;
        task->direction = 1;

        task->current_zone_width = task->base_zone_width;

        float center = 0.5f;
        task->success_min = center - task->current_zone_width / 2.0f;
        task->success_max = center + task->current_zone_width / 2.0f;
    }
}

void handle_letter_key(Task *task, SDL_Keycode key)
{
    char pressed = (char)SDL_toupper(key);
    char expected = task->target_string[task->current_index];

    if (pressed == expected)
        task->current_index++;
    else
        task->current_index = 0;
}

void handle_memory_key(Task *task, SDL_Keycode key)
{
    int input = -1;

    if (key == SDLK_UP)
        input = 0;
    if (key == SDLK_DOWN)
        input = 1;
    if (key == SDLK_LEFT)
        input = 2;
    if (key == SDLK_RIGHT)
        input = 3;

    if (input != -1)
    {
        if (input == task->sequence[task->input_index])
        {
            task->input_index++;

            if (task->input_index >= task->sequence_length)
            {
                // next round if success
                task->round++;

                if (task->round >= 3)
                {
                    end_task(task, TASK_STATUS_COMPLETED);
                }
                else
                {
                    task->sequence_length++;
                    task->flash_interval *= 0.8f;

                    task->flash_index = 0;
                    task->showing_sequence = true;
                    task->flash_timer = task->flash_interval;

                    for (int i = 0; i < task->sequence_length; i++)
                    {
                        task->sequence[i] = rand() % 4;
                    }
                }
            }
        }
        else
        {
            // reset if failed
            task->round = 0;
            task->sequence_length = 3;
            task->flash_interval = 0.6f;

            task->flash_index = 0;
            task->showing_sequence = true;
            task->flash_timer = task->flash_interval;

            for (int i = 0; i < task->sequence_length; i++)
            {
                task->sequence[i] = rand() % 4;
            }
        }
    }
}

void handle_hold_key(Task *task, SDL_Keycode key)
{
    if (key == SDLK_SPACE)
        task->hold_key_down = true;
}

void handle_alternate_key(Task *task, SDL_Keycode key)
{
    if (key == SDLK_a || key == SDLK_d)
    {
        // Måste växla – inte trycka samma knapp två gånger
        if (key != task->alternate_last_key)
        {
            task->alternate_count++;
            task->alternate_last_key = key;
        }
    }
}

void task_handle_keyup(Task *task, SDL_Keycode key)
{
    if (!task->active)
        return;

    if (task->type == TASK_HOLD && key == SDLK_SPACE)
    {
        handle_hold_keyup(task, key);
        return;
    }
}

void handle_hold_keyup(Task *task, SDL_Keycode key)
{
    if (key == SDLK_SPACE)
    {
        task->hold_key_down = false;
        task->hold_timer = 0.0f; // reset if released
    }
}

void task_handle_click(Task *task, int mx, int my, SDL_Renderer *renderer)
{
    if (!task || !task->active)
        return;

    if (task->type == TASK_CLICK)
    {
        handle_clicktask_click(task, mx, my);
        return;
    }

    if (task->type == TASK_LOGICAL_ORDER)
    {
        handle_logical_order_click(task, mx, my, renderer);
        return;
    }
}

void handle_clicktask_click(Task *task, int mx, int my)
{
    task->click_count++;
}

void handle_logical_order_click(Task *task, int mx, int my, SDL_Renderer *renderer)
{
    SDL_Point mouse_pos = {mx, my};

    for (int i = 0; i < 5; i++)
    {
        if (task->number_texts[i] != NULL && SDL_PointInRect(&mouse_pos, &task->numbers_rect[i]))
        {
            if (task->numbers[i] == task->sortedNumbers[task->next_expected_idx])
            {
                text_destroy(task->number_texts[i]);
                task->number_texts[i] = NULL;
                task->next_expected_idx++;

                if (task->next_expected_idx >= 5)
                {
                    end_task(task, TASK_STATUS_COMPLETED);
                }
            }
            else
            {
                // Reset
                start_logical_order_task(task, renderer);
            }
            break;
        }
    }
}