#include "emergency_meeting.h"
#include "game.h"
#include "game_map.h"

void emergency_meeting_view(SDL_Renderer *renderer, SDL_Texture *emergency_button_view)
{
    int width = 685;
    int height = 460;
    SDL_Rect picture_size = {(LOGICAL_SCREEN_WIDTH/2)-width/2,((LOGICAL_SCREEN_HEIGHT)/2)-height/2,width,height};
    SDL_RenderCopy(renderer,emergency_button_view,NULL,&picture_size);
}