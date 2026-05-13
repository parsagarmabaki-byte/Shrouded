#include "ip_config.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#endif

int get_local_ip_address(char *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size == 0)
        return 0;

#ifdef _WIN32
    snprintf(buffer, buffer_size, "127.0.0.1");
    return 0;
#else
    struct ifaddrs *interfaces = NULL;

    if (getifaddrs(&interfaces) != 0)
        return 0;

    for (struct ifaddrs *ifa = interfaces; ifa; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
            continue;

        if (ifa->ifa_flags & IFF_LOOPBACK)
            continue;

        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
        const char *ip = inet_ntop(AF_INET, &addr->sin_addr, buffer, (socklen_t)buffer_size);

        if (ip && strncmp(buffer, "169.254.", 8) != 0)
        {
            freeifaddrs(interfaces);
            return 1;
        }
    }

    freeifaddrs(interfaces);
    snprintf(buffer, buffer_size, "127.0.0.1");
    return 0;
#endif
}

SDL_Rect get_enter_ip_rect(SDL_Renderer *renderer, SDL_Texture *enter_ip_background)
{
    int window_width;
    int window_height;
    int texture_width = 1600;
    int texture_height = 1000;

    SDL_GetRendererOutputSize(renderer, &window_width, &window_height);

    if (enter_ip_background)
        SDL_QueryTexture(enter_ip_background, NULL, NULL, &texture_width, &texture_height);

    float max_width = window_width * 0.68f;
    float max_height = window_height * 0.68f;
    float scale = max_width / texture_width;

    if (texture_height * scale > max_height)
        scale = max_height / texture_height;

    SDL_Rect rect = {
        (window_width - (int)(texture_width * scale)) / 2,
        (window_height - (int)(texture_height * scale)) / 2,
        (int)(texture_width * scale),
        (int)(texture_height * scale)
    };

    return rect;
}

SDL_Rect get_my_ip_button_rect(SDL_Rect input_box)
{
    SDL_Rect rect = {
        input_box.x + (int)(input_box.w * 0.690f),
        input_box.y + (int)(input_box.h * 0.210f),
        (int)(input_box.w * 0.290f),
        (int)(input_box.h * 0.580f)
    };

    return rect;
}

int point_in_rect(int x, int y, SDL_Rect rect)
{
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

void render_my_ip_button(SDL_Renderer *renderer, Text button_text, SDL_Rect button_rect)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 18, 15, 11, 230);
    SDL_RenderFillRect(renderer, &button_rect);

    SDL_SetRenderDrawColor(renderer, 222, 181, 90, 255);
    SDL_RenderDrawRect(renderer, &button_rect);

    SDL_Rect inner = {
        button_rect.x + 2,
        button_rect.y + 2,
        button_rect.w - 4,
        button_rect.h - 4
    };
    SDL_SetRenderDrawColor(renderer, 86, 64, 31, 255);
    SDL_RenderDrawRect(renderer, &inner);

    text_draw(button_text,
              button_rect.x + button_rect.w / 2,
              button_rect.y + button_rect.h / 2);
}

void render_connection_background(SDL_Renderer *renderer, SDL_Texture *enter_ip_background, SDL_Texture *fallback_background, SDL_Rect enter_ip_rect)
{
    int width;
    int height;

    SDL_RenderCopy(renderer, fallback_background, NULL, NULL);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);
    SDL_GetRendererOutputSize(renderer, &width, &height);
    SDL_Rect full_screen = {0, 0, width, height};
    SDL_RenderFillRect(renderer, &full_screen);

    if (enter_ip_background)
        SDL_RenderCopy(renderer, enter_ip_background, NULL, &enter_ip_rect);
}
