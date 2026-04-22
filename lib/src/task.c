#include "task.h"

static const SDL_Color WHITE = {255,255,255,255};

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


void init_task(Task *task, SDL_Renderer *renderer)
{
    task->type = TASK_NONE;
    task->active = false;
    task->timer = 0.0f;
    task->points = 0; //total crewmate points
    task->font = TTF_OpenFont("assets/fonts/BebasNeue-Regular.ttf", 32);
    if (!task->font)
    {
        printf("Font error: %s\n", TTF_GetError());
    }
    if (task->font)
    {
        task->global_text_texture = create_text_texture(renderer, task->font, "PRESS Q TO CLOSE ASSIGNMENT", WHITE, &task->global_text_w, &task->global_text_h);
    }
    else
    {
        task->global_text_texture = NULL;
    }
    task->task_text_texture = NULL;
    task->task_image = NULL;
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
    task->success_min = 0.4f;
    task->success_max = 0.6f;
    task->success_count = 0;
    task->success_target = 5;

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
        task->task_text_texture = create_text_texture(renderer, task->font, "STOKE THE FIRE", WHITE, &task->task_text_w, &task->task_text_h);
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
                complete_task(task);
            }
            break;
        case TASK_CLICK:
            if (task->click_count >= task->click_target)
            {
                complete_task(task);
            }
            break;
        case TASK_TYPE:
            if (task->current_index >= task->length)
            {
                complete_task(task);
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
        default:
            break;
    }
}

void complete_task(Task *task)
{
    task->active = false;
    task->type = TASK_NONE;
    task->points += 1;
    cleanup_task(task);
}

void cancel_task(Task *task)
{
    if (!task->active) return;

    task->active = false;
    task->type = TASK_NONE;
    cleanup_task(task);
}

void cleanup_task(Task *task) // cleans non specific things
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
}

void destroy_task(Task *task) // cleans everything, used at the end of the game
{
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
            
            break;
        }

        default:
            break;
    }
}