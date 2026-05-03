#ifndef TEXT_H
#define TEXT_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct Text *Text;

// Initierar SDL_ttf. Anropas EN gång vid programstart.
bool text_init(void);

// Stänger ner SDL_ttf. Anropas vid programavslut.
void text_quit(void);

// Skapar ett Text-objekt från en .ttf-fil och en storlek (pt).
Text text_create(SDL_Renderer *renderer, const char *font_path, int size);

// Uppdaterar texten som ska visas. Skapar ny texture internt.
bool text_set(Text t, const char *content, SDL_Color color);

// Ritar texten centrerad på (x, y).
void text_draw(Text t, int x, int y);

// Ritar texten med (x, y) som övre vänstra hörnet.
void text_draw_at(Text t, int x, int y);

// Hämta bredd/höjd på den senast satta texten.
int text_get_width(Text t);
int text_get_height(Text t);

// Frigör allt minne och texturer.
void text_destroy(Text t);

#endif
