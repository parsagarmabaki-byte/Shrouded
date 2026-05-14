#include "task_render.h"
#include "task_internal.h"

const SDL_Color WHITE = {255, 255, 255, 255};

// Relative positioning system
typedef struct {
    float x_ratio;  // relative to box center (0.5 = center, 0 = left, 1 = right)
    float y_ratio;  // relative to box top (0 = top, 1 = bottom)
} UIPosition;

// Calculate screen position from box-relative position
void get_relative_position(int box_x, int box_y, int box_width, int box_height, UIPosition rel_pos, int *out_x, int *out_y)
{
    *out_x = box_x + (int)(box_width * rel_pos.x_ratio);
    *out_y = box_y + (int)(box_height * rel_pos.y_ratio);
}

// Position constants for each task type
// Format: x_ratio (0.5 = center), y_ratio (0 = top, 1 = bottom)
static const UIPosition POS1 = {0.5f, 0.3f};
static const UIPosition POS2 = {0.5f, 0.45f};
static const UIPosition POS3 = {0.5f, 0.55f};
static const UIPosition POS4 = {0.5f, 0.20f};
static const UIPosition POS5 = {0.02f, 0.91f};

void draw_centered_text(Text text, const char *content, SDL_Color color, int box_x, int box_y, int box_width, int box_height, UIPosition pos)
{
    text_set(text, content, color);
    int text_x, text_y;
    get_relative_position(box_x, box_y, box_width, box_height, pos, &text_x, &text_y);
    text_x -= text_get_width(text) / 2; // center text horizontally
    text_draw_at(text, text_x, text_y);
}

void render_task(SDL_Renderer *renderer, Task *task, int screen_width, int screen_height)
{
    if (!task->active)
        return;

    // Safety checks for required text objects
    if (!task->task_text || !task->dynamic_text)
    {
        printf("Warning: task_text or dynamic_text is NULL in render_task\n");
        return;
    }

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
        {
            int global_x, global_y;
            get_relative_position(box_x, box_y, box_width, box_height, POS5, &global_x, &global_y);
            text_draw_at(task->global_text, global_x, global_y);
        }

    switch (task->type)
    {
    case TASK_TIMER:
        render_timer_task(renderer, task, box_x, box_y, box_width, box_height);
        break;
    case TASK_CLICK:
        render_click_task(renderer, task, box_x, box_y, box_width, box_height);
        break;
    case TASK_LETTER:
        render_letter_task(renderer, task, box_x, box_y, box_width, box_height);
        break;
    case TASK_REFLEX:
        render_reflex_task(renderer, task, box_x, box_y, box_width, box_height);
        break;
    case TASK_LOGICAL_ORDER:
        render_logical_order_task(renderer, task, box_x, box_y, box_width, box_height);
        break;
    case TASK_MEMORY:
        render_memory_task(renderer, task, box_x, box_y, box_width, box_height);
        break;
    case TASK_HOLD:
        render_hold_task(renderer, task, box_x, box_y, box_width, box_height);
        break;
    case TASK_ALTERNATE:
        render_alternate_task(renderer, task, box_x, box_y, box_width, box_height);
        break;
    default:
        break;
    }
}

void render_timer_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height)
{
    // progress bar calculation
    float progress = 0.0f;
    if (task->timer_duration > 0.0f)
    {
        progress = 1.0f - (task->timer / task->timer_duration);
    }

    // progress bar background - relative positioning
    int bar_x, bar_y;
    get_relative_position(box_x, box_y, box_width, box_height, POS2, &bar_x, &bar_y);
    
    int bar_width = (int)(box_width * 0.28f);
    int bar_height = (int)(box_height * 0.08f);
    bar_x -= bar_width / 2;  // center the bar

    SDL_Rect bar_bg = {bar_x, bar_y, bar_width, bar_height};
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &bar_bg);

    // progress bar green fill
    SDL_Rect bar_fill = {bar_x, bar_y, (int)(bar_width * progress), bar_height};
    SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
    SDL_RenderFillRect(renderer, &bar_fill);

    // instruction text
    if (task->task_text)
    {
        int text_x, text_y;
        get_relative_position(box_x, box_y, box_width, box_height, POS1, &text_x, &text_y);
        text_x -= text_get_width(task->task_text) / 2; // center text
        text_draw_at(task->task_text, text_x, text_y);
    }
}

void render_click_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d / %d", task->click_count, task->click_target);

    if (task->dynamic_text)
    {
        draw_centered_text(task->dynamic_text, buffer, WHITE, box_x, box_y, box_width, box_height, POS2);
    }

    // instruction text
    if (task->task_text)
    {
        int instr_x, instr_y;
        get_relative_position(box_x, box_y, box_width, box_height, POS1, &instr_x, &instr_y);
        instr_x -= text_get_width(task->task_text) / 2;
        text_draw_at(task->task_text, instr_x, instr_y);
    }
}

void render_letter_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height)
{
    char buffer[32];

    // show progress
    for (int i = 0; i < task->length; i++)
    {
        if (i < task->current_index)
            buffer[i] = task->target_string[i];
        else
            buffer[i] = '_';
    }
    buffer[task->length] = '\0';

    if (task->dynamic_text)
    {
        draw_centered_text(task->dynamic_text, buffer, WHITE, box_x, box_y, box_width, box_height, POS3);
    }

    // show target string (next letter)
    char current[2];
    if (task->current_index < task->length)
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
        // SET TEXT FIRST
        text_set(task->dynamic_text, current, WHITE);
        
        // THEN get width and calculate position
        int next_x, next_y;
        get_relative_position(box_x, box_y, box_width, box_height, POS2, &next_x, &next_y);
        next_x -= text_get_width(task->dynamic_text) / 2;
        
        text_draw_at(task->dynamic_text, next_x, next_y);
    }

    // instruction text
    if (task->task_text)
    {
        int instr_x, instr_y;
        get_relative_position(box_x, box_y, box_width, box_height, POS1, &instr_x, &instr_y);
        instr_x -= text_get_width(task->task_text) / 2;
        text_draw_at(task->task_text, instr_x, instr_y);
    }
}

void render_reflex_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height)
{
    // bar background
    int bar_x, bar_y;
    get_relative_position(box_x, box_y, box_width, box_height, POS2, &bar_x, &bar_y);
    
    int bar_width = (int)(box_width * 0.42f);
    int bar_height = (int)(box_height * 0.08f);
    bar_x -= bar_width / 2;

    SDL_Rect bar_bg = {bar_x, bar_y, bar_width, bar_height};
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &bar_bg);

    // success zone
    int success_x = bar_x + (int)(task->success_min * bar_width);
    int success_w = (int)((task->success_max - task->success_min) * bar_width);

    SDL_Rect success_zone = {success_x, bar_y, success_w, bar_height};
    SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
    SDL_RenderFillRect(renderer, &success_zone);

    // moving cursor
    int cursor_x = bar_x + (int)(task->cursor_pos * bar_width);

    SDL_Rect cursor = {cursor_x - 5, bar_y - 5, 10, bar_height + 10};
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
    SDL_RenderFillRect(renderer, &cursor);

    // progress text (success count)
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d / %d", task->success_count, task->success_target);

    if (task->dynamic_text)
    {
        draw_centered_text(task->dynamic_text, buffer, WHITE, box_x, box_y, box_width, box_height, POS4);
    }

    // instruction text
    if (task->task_text)
    {
        int instr_x, instr_y;
        get_relative_position(box_x, box_y, box_width, box_height, POS1, &instr_x, &instr_y);
        instr_x -= text_get_width(task->task_text) / 2;
        text_draw_at(task->task_text, instr_x, instr_y);
    }
}

void render_logical_order_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height)
{
    // Calculate number positions relative to box
    int num_start_x = box_x + (int)(box_width * 0.20f);  // 20% from left edge
    int num_spacing = (int)(box_width * 0.14f);          // 14% spacing
    int num_y = box_y + (int)(box_height * 0.45f);       // 45% down

    for (int i = 0; i < 5; i++)
    {
        if (task->number_texts[i] != NULL)
        {
            task->numbers_rect[i].x = num_start_x + (i * num_spacing);
            task->numbers_rect[i].y = num_y;
            text_draw_at(task->number_texts[i], task->numbers_rect[i].x, task->numbers_rect[i].y);
        }
    }

    // show score
    char progress_buf[16];
    snprintf(progress_buf, sizeof(progress_buf), "%d / 5", task->next_expected_idx);

    if (task->dynamic_text)
    {
        draw_centered_text(task->dynamic_text, progress_buf, WHITE, box_x, box_y, box_width, box_height, POS4);
    }

    // instruction text
    if (task->task_text)
    {
        int instr_x, instr_y;
        get_relative_position(box_x, box_y, box_width, box_height, POS1, &instr_x, &instr_y);
        instr_x -= text_get_width(task->task_text) / 2;
        text_draw_at(task->task_text, instr_x, instr_y);
    }
}

void render_memory_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height)
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

                int seq_x, seq_y;
                get_relative_position(box_x, box_y, box_width, box_height, POS2, &seq_x, &seq_y);
                seq_x -= text_get_width(task->dynamic_text) / 2;
                
                text_draw_at(task->dynamic_text, seq_x, seq_y);
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
            draw_centered_text(task->dynamic_text, buffer, WHITE, box_x, box_y, box_width, box_height, POS3);
        }
    }

    // round text
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Round %d / 3", task->round + 1);

    if (task->dynamic_text)
    {
        draw_centered_text(task->dynamic_text, buffer, WHITE, box_x, box_y, box_width, box_height, POS4);
    }

    // instruction text
    if (task->task_text)
    {
        int instr_x, instr_y;
        get_relative_position(box_x, box_y, box_width, box_height, POS1, &instr_x, &instr_y);
        instr_x -= text_get_width(task->task_text) / 2;
        text_draw_at(task->task_text, instr_x, instr_y);
    }
}

void render_hold_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height)
{
    float progress = task->hold_timer / task->hold_duration;

    // Bar background
    int bar_x, bar_y;
    get_relative_position(box_x, box_y, box_width, box_height, POS2, &bar_x, &bar_y);
    
    int bar_width = (int)(box_width * 0.42f);
    int bar_height = (int)(box_height * 0.08f);
    bar_x -= bar_width / 2;

    SDL_Rect bar_bg = {bar_x, bar_y, bar_width, bar_height};
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &bar_bg);

    // Bar fill
    SDL_Rect bar_fill = {bar_x, bar_y, (int)(bar_width * progress), bar_height};
    SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
    SDL_RenderFillRect(renderer, &bar_fill);

    // Text
    if (task->task_text)
    {
        int instr_x, instr_y;
        get_relative_position(box_x, box_y, box_width, box_height, POS1, &instr_x, &instr_y);
        instr_x -= text_get_width(task->task_text) / 2;
        text_draw_at(task->task_text, instr_x, instr_y);
    }

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.1f / %.1f s", task->hold_timer, task->hold_duration);
    if (task->dynamic_text)
    {
        draw_centered_text(task->dynamic_text, buffer, WHITE, box_x, box_y, box_width, box_height, POS3);
    }
}

void render_alternate_task(SDL_Renderer *renderer, Task *task, int box_x, int box_y, int box_width, int box_height)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d / %d", task->alternate_count, task->alternate_target);

    if (task->dynamic_text)
    {
        draw_centered_text(task->dynamic_text, buffer, WHITE, box_x, box_y, box_width, box_height, POS3);
    }

    // Show which button to press next
    const char *next_key = (task->alternate_last_key == SDLK_a) ? "D" : "A";
    if (task->dynamic_text)
    {
        draw_centered_text(task->dynamic_text, next_key, WHITE, box_x, box_y, box_width, box_height, POS2);
    }

    if (task->task_text)
    {
        int instr_x, instr_y;
        get_relative_position(box_x, box_y, box_width, box_height, POS1, &instr_x, &instr_y);
        instr_x -= text_get_width(task->task_text) / 2;
        text_draw_at(task->task_text, instr_x, instr_y);
    }
}