#ifndef TASK_INTERNAL_H
#define TASK_INTERNAL_H

#include "task.h"
#include "text.h"
#include <SDL2/SDL_ttf.h>

struct Task
{ // task ADT struct
    TaskType type;
    TaskStatus status;
    bool active;
    SDL_Texture *task_image;
    TaskType last_completed_type;

    // text
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
    float cursor_pos; // 0.0 → 1.0
    float cursor_speed;
    int direction;     // 1 or -1
    float success_min; // cursor range for success
    float success_max;
    float base_zone_width; // success zone width for shrinking
    float current_zone_width;
    int success_count;
    int success_target; // number of wins to complete task

    // TASK_LOGICAL_ORDER specific
    int numbers[5];
    int sortedNumbers[5];
    int next_expected_idx;
    Text number_texts[5];
    SDL_Rect numbers_rect[5];

    // TASK_MEMORY specific
    int sequence[8];     // directions (0–3)
    int sequence_length; // 3-5
    int input_index;
    int round;
    float start_delay;
    float flash_timer;
    int flash_index;
    float flash_interval;
    bool showing_sequence;
    bool flash_visible;

    // TASK_HOLD specific
    float hold_timer;
    float hold_duration;
    bool hold_key_down;

    // TASK_ALTERNATE specific
    int alternate_count;
    int alternate_target;
    SDL_Keycode alternate_last_key; // senast tryckt tangent
};

#endif