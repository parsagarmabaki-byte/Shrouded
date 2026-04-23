#ifndef WALL_DATA_H_INCLUDED

#define WALL_DATA_H_INCLUDED

#include <stdbool.h>

#define WALL_TILE_SIZE 32
#define WALL_MAP_COLS  80
#define WALL_MAP_ROWS  64
#define HITBOX_SIZE 35

extern const unsigned char wall_map[WALL_MAP_ROWS][WALL_MAP_COLS];
int collides_with_wall(float x, float y);

#endif
