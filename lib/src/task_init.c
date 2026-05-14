#include "task_init.h"
#include <SDL2/SDL_image.h>
#include <time.h>


static void sort_numbers(int *arr, int size)
{
    for (int i = 0; i < size - 1; i++)
    {
        for (int j = 0; j < size - i - 1; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

static bool begin_task(Task *task, TaskType type, SDL_Renderer *renderer, const char *image_path, const char *instruction)
{
    cleanup_task(task);
    task->type = type;
    task->active = true;

    task->task_image = IMG_LoadTexture(renderer, image_path);
    if (!task->task_image)
    {
        printf("Failed to load task image: %s\n", IMG_GetError());
        task->active = false;
        task->type = TASK_NONE;
        return false;
    }

    if (!task->task_text)
    {
        printf("Error: task_text is NULL in begin_task\n");
        task->active = false;
        task->type = TASK_NONE;
        return false;
    }

    text_set(task->task_text, instruction, WHITE);
    return true;
}

void start_timer_task(Task *task, SDL_Renderer *renderer, float duration)
{
    if (!begin_task(task, TASK_TIMER, renderer, "assets/images/tasks/mannequin.png", "SCAN IN PROGRESS"))
        return;

    task->timer = duration;
    task->timer_duration = duration;
}

void start_click_task(Task *task, SDL_Renderer *renderer, int target)
{
    if (!begin_task(task, TASK_CLICK, renderer, "assets/images/tasks/crystal.png", "CLEAN THE CRYSTAL (CLICK!)"))
        return;
    
    task->click_count = 0;
    task->click_target = target;
}

void start_letter_task(Task *task, SDL_Renderer *renderer)
{
    if (!begin_task(task, TASK_LETTER, renderer, "assets/images/tasks/desk.png", "WRITE THE LETTER"))
        return;

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
}

void start_reflex_task(Task *task, SDL_Renderer *renderer)
{
    if (!begin_task(task, TASK_REFLEX, renderer, "assets/images/tasks/fireplace.png", "STOKE THE FIRE (PRESS SPACE!)"))
        return;

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
}

void start_logical_order_task(Task *task, SDL_Renderer *renderer)
{
    if (!begin_task(task, TASK_LOGICAL_ORDER, renderer, "assets/images/tasks/shelf.png", "ORGANIZE THE SHELF (ASCENDING ORDER)"))
            return;

    task->next_expected_idx = 0;

    for (int i = 0; i < 5; i++)
    {
        int num;
        bool unique;

        do
        {
            unique = true;
            num = (rand() % 100) + 1;

            for (int j = 0; j < i; j++)
            {
                if (task->numbers[j] == num)
                {
                    unique = false;
                    break;
                }
            }
        } while (!unique);

        task->numbers[i] = num;
        task->sortedNumbers[i] = num;

        char str[4];
        sprintf(str, "%d", num);
        task->number_texts[i] = text_create(task->renderer, "assets/fonts/BebasNeue-Regular.ttf", 32);
        if (!task->number_texts[i])
        {
            printf("Failed to create text for logical order task\n");
            cleanup_task(task);
            task->active = false;
            task->type = TASK_NONE;
            return;
        }
        text_set(task->number_texts[i], str, WHITE);

        task->numbers_rect[i].w = text_get_width(task->number_texts[i]);
        task->numbers_rect[i].h = text_get_height(task->number_texts[i]);

        int start_x = 450;
        int spacing = 100;

        task->numbers_rect[i].x = start_x + (i * spacing);
        task->numbers_rect[i].y = 350;
    }
    
    sort_numbers(task->sortedNumbers, 5);
}

void start_memory_task(Task *task, SDL_Renderer *renderer)
{
    if (!begin_task(task, TASK_MEMORY, renderer, "assets/images/tasks/twocrystals.png", "GAZE INTO THE CRYSTALS (REMEMBER THE SEQUENCE!)"))
        return;

    task->round = 0;
    task->sequence_length = 3;
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
}

void start_hold_task(Task *task, SDL_Renderer *renderer, float duration)
{
    if (!begin_task(task, TASK_HOLD, renderer, "assets/images/tasks/mess.png", "CLEAN THE MESS (HOLD SPACE!)"))
        return;

    task->hold_timer = 0.0f;
    task->hold_duration = duration;
    task->hold_key_down = false;
}

void start_alternate_task(Task *task, SDL_Renderer *renderer, int target)
{
    if (!begin_task(task, TASK_ALTERNATE, renderer, "assets/images/tasks/bulb.png", "REPLACE THE LIGHTBULB (MASH A AND D ALTERNATING!)"))
        return;

    task->alternate_count = 0;
    task->alternate_target = target;
    task->alternate_last_key = SDLK_UNKNOWN;
}