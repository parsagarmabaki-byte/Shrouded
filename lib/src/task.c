#include "task.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static const SDL_Color WHITE = {255,255,255,255};

struct Task {           // task ADT struct
    TaskType type;
    TaskStatus status;
    bool active;
    float timer;
    SDL_Texture *task_image;

    //text
    TTF_Font *font;
    SDL_Texture *task_text_texture;
    SDL_Texture *global_text_texture;
    int task_text_w;
    int task_text_h;
    int global_text_w;
    int global_text_h;

    // TASK_TIMER specific
    float timer_duration;

    // TASK_CLICK specific
    int click_count;
    int click_target;

    // TASK_TYPE specific
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
    SDL_Texture *number_textures[5];
    SDL_Rect numbers_rect[5];

    // TASK_MEMORY specific
    int sequence[8];        // directions (0–3)
    int sequence_length;
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

    if (task->type == TASK_TYPE)
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

void task_handle_click(Task *task)
{
    if (!task || !task->active) return;

    if (task->type == TASK_CLICK)
    {
        task->click_count++;
    }
}

SDL_Texture* create_text_texture(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color, int *w, int *h)
{
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color); // render text to surface

    if (!surface)
    {
        printf("Text render error: %s\n", TTF_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface); // convert surface to texture

    if (!texture)
    {
        printf("Texture creation error: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return NULL;
    }

    // store the width and height of the rendered text back in the caller’s variables
    *w = surface->w;
    *h = surface->h;

    SDL_FreeSurface(surface); // free the surface after creating texture
    return texture;
}


Task* create_task(SDL_Renderer *renderer)
{
    Task *task = malloc(sizeof(Task));
    if (!task) return NULL;

    task->type = TASK_NONE;
    task->active = false;
    task->timer = 0.0f;

    task->font = TTF_OpenFont("assets/fonts/BebasNeue-Regular.ttf", 32);

    if (task->font)
    {
        task->global_text_texture = create_text_texture(
            renderer, task->font,
            "PRESS Q TO CLOSE ASSIGNMENT",
            WHITE,
            &task->global_text_w,
            &task->global_text_h
        );
    }
    else
    {
        task->global_text_texture = NULL;
    }

    task->task_text_texture = NULL;
    task->task_image = NULL;

    for(int i = 0; i < 5; i++)
        task->number_textures[i] = NULL;

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
    if (task->task_text_texture)
    {
        SDL_DestroyTexture(task->task_text_texture);
        task->task_text_texture = NULL;
    }

    if (task->task_image)
    {
        SDL_DestroyTexture(task->task_image);
        task->task_image = NULL;
    }

    for(int i = 0; i < 5; i++)
    {
        if(task->number_textures[i])
        {
            SDL_DestroyTexture(task->number_textures[i]);
            task->number_textures[i] = NULL;
        }
    }
}

void destroy_task(Task *task) // cleans everything, used at the end of the game
{
    if (!task) return;

    cleanup_task(task);

    if (task->global_text_texture)
    {
        SDL_DestroyTexture(task->global_text_texture);
        task->global_text_texture = NULL;
    }

    if (task->font)
    {
        TTF_CloseFont(task->font);
        task->font = NULL;
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

    task->task_image = IMG_LoadTexture(renderer, "assets/images/mannequin.png");
    if (!task->task_image)
    {
        printf("Failed to load timer task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
    }

    if (!task->font)
    {
        printf("Font not loaded, cannot create text\n");
    }
    else
    {
        task->task_text_texture = create_text_texture(renderer, task->font, "SCAN IN PROGRESS", WHITE, &task->task_text_w, &task->task_text_h);
    }
}

void start_click_task(Task *task, SDL_Renderer *renderer, int target)
{
    cleanup_task(task);
    task->type = TASK_CLICK;
    task->active = true;

    task->click_count = 0;
    task->click_target = target;

    task->task_image = IMG_LoadTexture(renderer, "assets/images/crystal.png");
    if (!task->task_image)
    {
        printf("Failed to load click task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
    }

    if (!task->font)
    {
        printf("Font not loaded, cannot create text\n");
    }
    else
    {
        task->task_text_texture = create_text_texture(renderer, task->font, "CLEAN THE CRYSTAL (Click!)", WHITE, &task->task_text_w, &task->task_text_h);
    }
}

void start_type_task(Task *task, SDL_Renderer *renderer)
{
    cleanup_task(task);
    task->type = TASK_TYPE;
    task->active = true;

    const char *letters = "ABCDEFGHIJKLMNOPRSTUVWXYZ";
    int len = 10;
    task->length = len;
    task->current_index = 0;

    // create random string of "len" letters
    for (int i = 0; i < len; i++)
    {
        task->target_string[i] = letters[rand() % 25];
    }
    task->target_string[len] = '\0';

    task->task_image = IMG_LoadTexture(renderer, "assets/images/desk.png");
    if (!task->task_image)
    {
        printf("Failed to load type task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
    }

    if (!task->font)
    {
        printf("Font not loaded, cannot create text\n");
    }
    else
    {
        task->task_text_texture = create_text_texture(renderer, task->font, "WRITE LETTER", WHITE, &task->task_text_w, &task->task_text_h);
    }
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

    task->task_image = IMG_LoadTexture(renderer, "assets/images/fireplace.png");
    if (!task->task_image)
    {
        printf("Failed to load timer task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
    }

    if (!task->font)
    {
        printf("Font not loaded, cannot create text\n");
    }
    else
    {
        task->task_text_texture = create_text_texture(renderer, task->font, "STOKE THE FIRE (PRESS SPACE!)", WHITE, &task->task_text_w, &task->task_text_h);
    }
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
        task->number_textures[i] = create_text_texture(renderer, task->font, str, white, &task->numbers_rect[i].w, &task->numbers_rect[i].h);

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

    task->task_image = IMG_LoadTexture(renderer, "assets/images/twocrystals.png");
    if (!task->task_image)
    {
        printf("Failed to load timer task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
    }

    if (task->font)
    {
        task->task_text_texture = create_text_texture(
            renderer,
            task->font,
            "GAZE INTO THE CRYSTALS (REMEMBER THE SEQUENCE)",
            WHITE,
            &task->task_text_w,
            &task->task_text_h
        );
    }
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
        case TASK_TYPE:
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
        default:
            break;
    }
}

void render_task(SDL_Renderer *renderer, Task *task)
{
    if (!task->active) return;

    SDL_Rect box = {225, 150, 900, 500}; // adjust for screen size later for both text and image

    if (task->task_image)
    {
        SDL_RenderCopy(renderer, task->task_image, NULL, &box);
    }
    
    if (task->global_text_texture)
    {
        SDL_Rect rect = {170, 275, task->global_text_w, task->global_text_h};
        SDL_RenderCopy(renderer, task->global_text_texture, NULL, &rect);
    }
    

    switch (task->type)
    {
        case TASK_TIMER:
        {
            // progress bar calculation
            float progress = 0.0f;
            if (task->timer_duration > 0.0f)
            {
                progress = task->timer / task->timer_duration;
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
            if (task->task_text_texture)
            {
                SDL_Rect textRect = {520, 400, task->task_text_w, task->task_text_h};
                SDL_RenderCopy(renderer, task->task_text_texture, NULL, &textRect);
            }
            break;
        }

        case TASK_CLICK:
        {
            // click counter dynamic text
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d / %d", task->click_count, task->click_target); // write string into the buffer

            SDL_Surface *surface = TTF_RenderText_Blended(task->font, buffer, WHITE); // render text in buffer to surface
            if (!surface) break;

            SDL_Texture *textTex = SDL_CreateTextureFromSurface(renderer, surface);
            if (!textTex)
            {
                SDL_FreeSurface(surface);
                break;
            }

            SDL_Rect textRect = {520, 400, surface->w, surface->h};

            SDL_RenderCopy(renderer, textTex, NULL, &textRect);

            // instruction text
            if (task->task_text_texture)
            {
                SDL_Rect textRect2 = {520, 350, task->task_text_w, task->task_text_h};
                SDL_RenderCopy(renderer, task->task_text_texture, NULL, &textRect2);
            }

            SDL_FreeSurface(surface);
            SDL_DestroyTexture(textTex);

            break;
        }

        case TASK_TYPE:
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

            SDL_Surface *surface = TTF_RenderText_Blended(task->font, buffer, WHITE);
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);

            SDL_Rect r = {520, 400, surface->w, surface->h};
            SDL_RenderCopy(renderer, tex, NULL, &r);

            SDL_FreeSurface(surface);
            SDL_DestroyTexture(tex);

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

            surface = TTF_RenderText_Blended(task->font, current, WHITE);
            tex = SDL_CreateTextureFromSurface(renderer, surface);

            SDL_Rect rect = {520, 300, surface->w, surface->h};
            SDL_RenderCopy(renderer, tex, NULL, &rect);

            SDL_FreeSurface(surface);
            SDL_DestroyTexture(tex);

            // instruction text
            if (task->task_text_texture)
            {
                SDL_Rect t = {520, 350, task->task_text_w, task->task_text_h};
                SDL_RenderCopy(renderer, task->task_text_texture, NULL, &t);
            }

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

            SDL_Surface *surface = TTF_RenderText_Blended(task->font, buffer, WHITE);
            if (surface)
            {
                SDL_Texture *textTex = SDL_CreateTextureFromSurface(renderer, surface);

                if (textTex)
                {
                    SDL_Rect textRect = {520, 400, surface->w, surface->h};
                    SDL_RenderCopy(renderer, textTex, NULL, &textRect);
                    SDL_DestroyTexture(textTex);
                }

                SDL_FreeSurface(surface);
            }

            // instruction text
            if (task->task_text_texture)
            {
                SDL_Rect textRect2 = {520, 300, task->task_text_w, task->task_text_h};
                SDL_RenderCopy(renderer, task->task_text_texture, NULL, &textRect2);
            }

            break;
        }
        
        case TASK_LOGICAL_ORDER:
        {
            for (int i = 0; i < 5; i++)
            {
                if (task->number_textures[i] != NULL)
                {
                    SDL_RenderCopy(renderer, task->number_textures[i], NULL, &task->numbers_rect[i]);
                }
            }
            
            // show score
            char progress_buf[16];
            snprintf(progress_buf, sizeof(progress_buf), "%d / 5", task->next_expected_idx);
            SDL_Surface *surf = TTF_RenderText_Blended(task->font, progress_buf, WHITE);
            if (surf) 
            {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect r = {520, 450, surf->w, surf->h};
                SDL_RenderCopy(renderer, tex, NULL, &r);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);
            }
    
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

                    SDL_Surface *surface = TTF_RenderText_Blended(task->font, symbol, WHITE);
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);

                    SDL_Rect r = {650, 350, surface->w, surface->h};
                    SDL_RenderCopy(renderer, tex, NULL, &r);

                    SDL_FreeSurface(surface);
                    SDL_DestroyTexture(tex);
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

                SDL_Surface *surface = TTF_RenderText_Blended(task->font, buffer, WHITE);
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);

                SDL_Rect r = {600, 400, surface->w, surface->h};
                SDL_RenderCopy(renderer, tex, NULL, &r);

                SDL_FreeSurface(surface);
                SDL_DestroyTexture(tex);
            }

            // round text
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "Round %d / 3", task->round + 1);

            SDL_Surface *surface = TTF_RenderText_Blended(task->font, buffer, WHITE);
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);

            SDL_Rect r = {520, 450, surface->w, surface->h};
            SDL_RenderCopy(renderer, tex, NULL, &r);

            SDL_FreeSurface(surface);
            SDL_DestroyTexture(tex);

            // instruction text
            if (task->task_text_texture)
            {
                SDL_Rect t = {520, 300, task->task_text_w, task->task_text_h};
                SDL_RenderCopy(renderer, task->task_text_texture, NULL, &t);
            }

            break;
        }

    default:
        break;
    }
}

void task_handle_logical_order(Task *task, int mx, int my, SDL_Renderer *renderer)
{
    if (!task || !task->active || task->type != TASK_LOGICAL_ORDER)
    {
        return;
    }

    SDL_Point mouse_pos = {mx, my};

    for (int i = 0; i < 5; i++)
    {
        if (task->number_textures[i] != NULL && SDL_PointInRect(&mouse_pos, &task->numbers_rect[i]))
        {
            if (task->numbers[i] == task->sortedNumbers[task->next_expected_idx])
            {
                SDL_DestroyTexture(task->number_textures[i]);
                task->number_textures[i] = NULL;
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