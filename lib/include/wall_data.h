#ifndef WALL_DATA_H_INCLUDED

#define WALL_DATA_H_INCLUDED

#include <stdbool.h>

#define WALL_TILE_SIZE 32
#define WALL_MAP_COLS  80
#define WALL_MAP_ROWS  64

extern const unsigned char wall_map[WALL_MAP_ROWS][WALL_MAP_COLS];
extern const unsigned char activity_map_position[WALL_MAP_ROWS][WALL_MAP_COLS];
bool collides_with_wall(const unsigned char map[WALL_MAP_ROWS][WALL_MAP_COLS] ,float x, float y, float w, float h);
bool activity_map(float x, float y, float w, float h);

#endif
