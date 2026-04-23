#ifndef TASK_H
#define TASK_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    TASK_NONE,
    TASK_TIMER,
    TASK_CLICK,
    TASK_TYPE,
    TASK_REFLEX
} TaskType;

typedef struct {
    TaskType type;
    bool active;
    int points;
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
    float current_zone_width;;
    int success_count;    
    int success_target;   // number of wins to complete task

} Task;


SDL_Texture* create_text_texture(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color, int *w, int *h);
void init_task(Task *task, SDL_Renderer *renderer);
void start_timer_task(Task *task, SDL_Renderer *renderer, float duration);
void start_click_task(Task *task, SDL_Renderer *renderer, int target);
void start_type_task(Task *task, SDL_Renderer *renderer);
void start_reflex_task(Task *task, SDL_Renderer *renderer);
void update_task(Task *task, float dt);
void complete_task(Task *task);
void cleanup_task(Task *task);
void destroy_task(Task *task);
void render_task(SDL_Renderer *renderer, Task *task);
void cancel_task(Task *task);

#endif