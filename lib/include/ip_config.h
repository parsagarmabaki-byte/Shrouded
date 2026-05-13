
#ifndef IP_CONFIG_H
#define IP_CONFIG_H

#include <stddef.h>
#include <SDL2/SDL.h>
#include "text.h"

int get_local_ip_address(char *buffer, size_t buffer_size);
SDL_Rect get_enter_ip_rect(SDL_Renderer *renderer, SDL_Texture *enter_ip_background);
SDL_Rect get_my_ip_button_rect(SDL_Rect input_box);
int point_in_rect(int x, int y, SDL_Rect rect);
void render_my_ip_button(SDL_Renderer *renderer, Text button_text, SDL_Rect button_rect);
void render_connection_background(SDL_Renderer *renderer, SDL_Texture *enter_ip_background, SDL_Texture *fallback_background, SDL_Rect enter_ip_rect);

#endif
