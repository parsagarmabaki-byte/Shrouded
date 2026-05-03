#include "task.h"
#include "text.h"
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static const SDL_Color WHITE = {255,255,255,255};

struct Task {           // task ADT struct
    TaskType type;
    TaskStatus status;
    bool active;
    SDL_Texture *task_image;

    //text
    SDL_Renderer *renderer;
    Text global_text;
    Text task_text;
    Text dynamic_text;

    // TASK_TIMER specific
    float timer;
    float timer_duration;

    // TASK_CLICK specific
    int click_count;
    int click_target;

    // TASK_LETTER specific
    char target_string[16]; 
    int current_index;
    int length;

    // TASK_REFLEX specific
    float cursor_pos;     // 0.0 → 1.0
    float cursor_speed;
    int direction;        // 1 or -1
    float success_min;    // cursor range for success
    float success_max;    
    float base_zone_width;  // success zone width for shrinking
    float current_zone_width;
    int success_count;    
    int success_target;   // number of wins to complete task

    // TASK_LOGICAL_ORDER specific
    int numbers[5];
    int sortedNumbers[5];
    int next_expected_idx;
    Text number_texts[5];
    SDL_Rect numbers_rect[5];

    // TASK_MEMORY specific
    int sequence[8];        // directions (0–3)
    int sequence_length;    // 4-6
    int input_index;
    int round;
    float start_delay;
    float flash_timer; 
    int flash_index;
    float flash_interval;
    bool showing_sequence;
    bool flash_visible;
};

// handle input events for tasks
void task_handle_key(Task *task, SDL_Keycode key)
{
    if (!task->active) return;

    if (task->type == TASK_REFLEX && key == SDLK_SPACE)
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

    if (task->type == TASK_LETTER)
    {
        char pressed = (char)SDL_toupper(key);
        char expected = task->target_string[task->current_index];

        if (pressed == expected)
            task->current_index++;
        else
            task->current_index = 0;
    }

    if (task->type == TASK_MEMORY && !task->showing_sequence)
    {
        int input = -1;

        if (key == SDLK_UP) input = 0;
        if (key == SDLK_DOWN) input = 1;
        if (key == SDLK_LEFT) input = 2;
        if (key == SDLK_RIGHT) input = 3;

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
                task->sequence_length = 4;
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
}

void task_handle_click(Task *task, int mx, int my, SDL_Renderer *renderer)
{
    if (!task || !task->active) return;

    if (task->type == TASK_CLICK)
    {
        task->click_count++;
    }

    if (task->type == TASK_LOGICAL_ORDER)
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
}

Task* create_task(SDL_Renderer *renderer)
{
    Task *task = malloc(sizeof(Task));
    if (!task) return NULL;

    task->type = TASK_NONE;
    task->active = false;
    task->timer = 0.0f;
    task->renderer = renderer;

    // Initialize Text objects
    task->global_text = text_create(renderer, "assets/fonts/BebasNeue-Regular.ttf", 32);
    task->task_text = text_create(renderer, "assets/fonts/BebasNeue-Regular.ttf", 32);
    task->dynamic_text = text_create(renderer, "assets/fonts/BebasNeue-Regular.ttf", 32);

    if (task->global_text)
        text_set(task->global_text, "PRESS Q TO ABANDON ASSIGNMENT", WHITE);

    // Initialize number_texts array to NULL
    for (int i = 0; i < 5; i++)
        task->number_texts[i] = NULL;

    return task;
}

bool task_active_check(Task *task) // call these 3 getter functions outside task.c to access struct variables
{
    return task->active;
}

TaskType task_get_current_type(Task *task)
{
    return task->type;
}

TaskStatus task_get_status(Task *task)
{
    return task->status;
}

void end_task(Task *task, TaskStatus status)
{
    task->active = false;
    task->type = TASK_NONE;
    task->status = status;  // Track HOW it ended
    cleanup_task(task);
}

void cleanup_task(Task *task) // cleans non specific things, used before starting a new task to free old textures and reset variables
{
    if (task->task_image)
    {
        SDL_DestroyTexture(task->task_image);
        task->task_image = NULL;
    }

    for(int i = 0; i < 5; i++)
    {
        if(task->number_texts[i])
        {
            text_destroy(task->number_texts[i]);
            task->number_texts[i] = NULL;
        }
    }
}

void destroy_task(Task *task) // cleans everything, used at the end of the game
{
    if (!task) return;

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

void start_timer_task(Task *task, SDL_Renderer *renderer, float duration)
{
    cleanup_task(task);
    task->type = TASK_TIMER;
    task->active = true;

    task->timer = duration;
    task->timer_duration = duration;

    task->task_image = IMG_LoadTexture(renderer, "assets/images/tasks/mannequin.png");
    if (!task->task_image)
    {
        printf("Failed to load timer task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
    }

    if (task->task_text)
        text_set(task->task_text, "SCAN IN PROGRESS", WHITE);
}

void start_click_task(Task *task, SDL_Renderer *renderer, int target)
{
    cleanup_task(task);
    task->type = TASK_CLICK;
    task->active = true;

    task->click_count = 0;
    task->click_target = target;

    task->task_image = IMG_LoadTexture(renderer, "assets/images/tasks/crystal.png");
    if (!task->task_image)
    {
        printf("Failed to load click task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
    }

    if (task->task_text)
        text_set(task->task_text, "CLEAN THE CRYSTAL (CLICK!)", WHITE);
}

void start_letter_task(Task *task, SDL_Renderer *renderer)
{
    cleanup_task(task);
    task->type = TASK_LETTER;
    task->active = true;

    const char *letters = "ABCDEFGHIJKLNOPRSTUVWXYZ";
    int len = 10;
    task->length = len;
    task->current_index = 0;

    // create random string of "len" letters
    for (int i = 0; i < len; i++)
    {
        task->target_string[i] = letters[rand() % 24];
    }
    task->target_string[len] = '\0';

    task->task_image = IMG_LoadTexture(renderer, "assets/images/tasks/desk.png");
    if (!task->task_image)
    {
        printf("Failed to load type task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
    }

    if (task->task_text)
        text_set(task->task_text, "WRITE THE LETTER", WHITE);
}

void start_reflex_task(Task *task, SDL_Renderer *renderer)
{
    cleanup_task(task);
    task->type = TASK_REFLEX;
    task->active = true;

    task->cursor_pos = 0.0f;
    task->cursor_speed = 0.8f;
    task->direction = 1;

    task->success_count = 0;
    task->success_target = 5;
    
    // success zone size
    float base_width = 0.2f;
    task->success_min = 0.4f; 
    task->success_max = 0.6f;
    task->base_zone_width = base_width;
    task->current_zone_width = base_width;

    task->task_image = IMG_LoadTexture(renderer, "assets/images/tasks/fireplace.png");
    if (!task->task_image)
    {
        printf("Failed to load timer task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
    }
 
    if (task->task_text)
        text_set(task->task_text, "STOKE THE FIRE (PRESS SPACE!)", WHITE);
}

void start_logical_order_task(Task *task, SDL_Renderer *renderer)
{
    srand(time(NULL)); 

    cleanup_task(task);

    task->type = TASK_LOGICAL_ORDER;
    task->active = true;
    task->next_expected_idx = 0;

    SDL_Color white = {255, 255, 255, 255};

    for(int i = 0; i < 5; i++)
    {
        int num;
        bool unique;

        do
        {
            unique = true;
            num = (rand() % 100) + 1;

            for(int j = 0; j < i; j++)
            {
                if(task->numbers[j] == num)
                {
                    unique = false;
                    break;
                }
            }
        } while(!unique);

        task->numbers[i] = num;
        task->sortedNumbers[i] = num;

        char str[4]; 
        sprintf(str, "%d", num);
        task->number_texts[i] = text_create(task->renderer, "assets/fonts/BebasNeue-Regular.ttf", 32);
        text_set(task->number_texts[i], str, white);

        task->numbers_rect[i].w = text_get_width(task->number_texts[i]);
        task->numbers_rect[i].h = text_get_height(task->number_texts[i]);

        int start_x = 450; 
        int spacing = 100; 

        task->numbers_rect[i].x = start_x + (i * spacing);
        task->numbers_rect[i].y = 350;
    }

    for(int i = 0; i < 5 - 1; i++)
    {
        for(int j = 0; j < 5 - i - 1; j++)
        {
            if(task->sortedNumbers[j] > task->sortedNumbers[j + 1])
            {
                int tmp = task->sortedNumbers[j];
                task->sortedNumbers[j] = task->sortedNumbers[j + 1];
                task->sortedNumbers[j + 1] = tmp;
            }
        }
    }

    task->task_image = IMG_LoadTexture(renderer, "assets/images/tasks/shelf.png");
    if (!task->task_image)
    {
        printf("Failed to load timer task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
    }

    if (task->task_text)
        text_set(task->task_text, "ORGANIZE THE SHELF (ASCENDING ORDER)", WHITE);
}

void start_memory_task(Task *task, SDL_Renderer *renderer)
{
    cleanup_task(task);
    task->type = TASK_MEMORY;
    task->active = true;

    task->round = 0;
    task->sequence_length = 4;
    task->start_delay = 2.0f;

    task->flash_visible = false;
    task->flash_interval = 0.6f;
    task->flash_index = -1;
    task->input_index = 0;

    task->showing_sequence = true;
    task->flash_timer = task->flash_interval;

    // generate sequence
    for (int i = 0; i < task->sequence_length; i++)
    {
        task->sequence[i] = rand() % 4;
    }

    task->task_image = IMG_LoadTexture(renderer, "assets/images/tasks/twocrystals.png");
    if (!task->task_image)
    {
        printf("Failed to load timer task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
    }

    if (task->task_text)
        text_set(task->task_text, "GAZE INTO THE CRYSTALS (REMEMBER THE SEQUENCE!)", WHITE);
}

void update_task(Task *task, float dt) // updates task logic every frame
{
    if (!task->active) return;

    switch (task->type)
    {
        case TASK_TIMER:
            task->timer -= dt;
            if (task->timer <= 0.0f)
            {
                end_task(task, TASK_STATUS_COMPLETED);
            }
            break;
        case TASK_CLICK:
            if (task->click_count >= task->click_target)
            {
                end_task(task, TASK_STATUS_COMPLETED);
            }
            break;
        case TASK_LETTER:
            if (task->current_index >= task->length)
            {
                end_task(task, TASK_STATUS_COMPLETED);
            }
            break;
        case TASK_REFLEX:
        {
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
            break;
        }
        case TASK_MEMORY:
        {
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
            break;
        }
        case TASK_LOGICAL_ORDER:
            // no frame-based logic needed, just here for clarity
            break;

        default:
            break;
    }
}

void render_task(SDL_Renderer *renderer, Task *task, int screen_width, int screen_height)
{
    if (!task->active) return;

    // center and size task image
    int box_width = (int)(screen_width * 0.70f);
    int box_height = (int)(box_width * screen_height / screen_width);
    int box_x = (screen_width - box_width) / 2;
    int box_y = (screen_height - box_height) / 2;

    SDL_Rect box = {box_x, box_y, box_width, box_height};

    if (task->task_image)
    {
        SDL_RenderCopy(renderer, task->task_image, NULL, &box);
    }
    
    if (task->global_text)
        text_draw_at(task->global_text, 170, 275);
    

    switch (task->type)
    {
        case TASK_TIMER:
        {
            // progress bar calculation
            float progress = 0.0f;
            if (task->timer_duration > 0.0f)
            {
                progress = 1.0f - (task->timer / task->timer_duration);
            }

            // progress bar background
            SDL_Rect bar_bg = {520, 350, 200, 40};
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &bar_bg);

            // progress bar green fill
            SDL_Rect bar_fill = {520, 350, (int)(200 * progress), 40};
            SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
            SDL_RenderFillRect(renderer, &bar_fill);

            // text
            if (task->task_text)
                text_draw_at(task->task_text, 520, 400);
            break;
        }

        case TASK_CLICK:
        {
            // click counter dynamic text
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d / %d", task->click_count, task->click_target); // write string into the buffer

            if (task->dynamic_text)
            {
                text_set(task->dynamic_text, buffer, WHITE);
                text_draw_at(task->dynamic_text, 520, 400);
            }

            // instruction text
            if (task->task_text)
                text_draw_at(task->task_text, 520, 350);

            break;
        }

        case TASK_LETTER:
        {
            char buffer[32];

            // show progress
            for (int i = 0; i < task->length; i++)
            {
                if (i < task->current_index)                // everything before current index is shown, rest is hidden
                    buffer[i] = task->target_string[i];
                else
                    buffer[i] = '_';
            }
            buffer[task->length] = '\0';

            if (task->dynamic_text)
            {
                text_set(task->dynamic_text, buffer, WHITE);
                text_draw_at(task->dynamic_text, 520, 400);
            }

            // show target string
            char current[2];

            if (task->current_index < task->length)        // if within defined text length, show next letter
            {
                current[0] = task->target_string[task->current_index];
            }
            else
            {
                current[0] = ' ';
            }

            current[1] = '\0';

            if (task->dynamic_text)
            {
                text_set(task->dynamic_text, current, WHITE);
                text_draw_at(task->dynamic_text, 520, 300);
            }

            // instruction text
            if (task->task_text)
                text_draw_at(task->task_text, 520, 350);

            break;
        }

        case TASK_REFLEX:
        {
            // bar background
            SDL_Rect bar_bg = {520, 350, 300, 40};
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &bar_bg);

            // success zone
            int success_x = 520 + (int)(task->success_min * 300);
            int success_w = (int)((task->success_max - task->success_min) * 300);

            SDL_Rect success_zone = {success_x, 350, success_w, 40};
            SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
            SDL_RenderFillRect(renderer, &success_zone);

            // moving cursor
            int cursor_x = 520 + (int)(task->cursor_pos * 300);

            SDL_Rect cursor = {cursor_x - 5, 345, 10, 50};
            SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
            SDL_RenderFillRect(renderer, &cursor);

            // progress text (success count)
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d / %d", task->success_count, task->success_target);

            if (task->dynamic_text)
            {
                text_set(task->dynamic_text, buffer, WHITE);
                text_draw_at(task->dynamic_text, 520, 400);
            }

            // instruction text
            if (task->task_text)
                text_draw_at(task->task_text, 520, 300);

            break;
        }
        
        case TASK_LOGICAL_ORDER:
        {
            for (int i = 0; i < 5; i++)
            {
                if (task->number_texts[i] != NULL)
                    text_draw_at(task->number_texts[i], task->numbers_rect[i].x, task->numbers_rect[i].y);
            }
            
            // show score
            char progress_buf[16];
            snprintf(progress_buf, sizeof(progress_buf), "%d / 5", task->next_expected_idx);
            
            if (task->dynamic_text)
            {
                text_set(task->dynamic_text, progress_buf, WHITE);
                text_draw_at(task->dynamic_text, 520, 450);
            }

            // instruction text
            if (task->task_text)
                text_draw_at(task->task_text, 520, 300);

            break;
    
            break;
        }

        case TASK_MEMORY:
        {
            const char *arrows[] = {"UP", "DOWN", "LEFT", "RIGHT"};

            // show sequence phase
            if (task->showing_sequence)
            {
                if (task->flash_visible && task->flash_index < task->sequence_length)
                {
                    const char *symbol = arrows[task->sequence[task->flash_index]];

                    if (task->dynamic_text)
                    {
                        text_set(task->dynamic_text, symbol, WHITE);
                        text_draw_at(task->dynamic_text, 650, 350);
                    }
                }
            }
            else
            {
                // input phase
                char buffer[16];

                for (int i = 0; i < task->sequence_length; i++)
                {
                    if (i < task->input_index)
                        buffer[i] = 'X';
                    else
                        buffer[i] = '_';
                }
                buffer[task->sequence_length] = '\0';

                if (task->dynamic_text)
                {
                    text_set(task->dynamic_text, buffer, WHITE);
                    text_draw_at(task->dynamic_text, 600, 400);
                }
            }

            // round text
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "Round %d / 3", task->round + 1);

            if (task->dynamic_text)
            {
                text_set(task->dynamic_text, buffer, WHITE);
                text_draw_at(task->dynamic_text, 520, 450);
            }

            // instruction text
            if (task->task_text)
                text_draw_at(task->task_text, 520, 300);

            break;
        }

    default:
        break;
    }
}