#ifndef KILL_ANIMATION_H
#define KILL_ANIMATION_H

typedef struct
{
    bool active;
    int killer_id;
    int victim_id;
    float x, y;
    int current_frame;
    float animation_timer;
} KillAnimation;

#endif