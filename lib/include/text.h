#ifndef TEXT_H
#define TEXT_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct Text *Text;

// Initierar SDL_ttf. Anropas EN gång vid programstart.
bool text_initiera(void);

// Stänger ner SDL_ttf. Anropas vid programavslut.
void text_avsluta(void);

// Skapar ett Text-objekt från en .ttf-fil och en storlek (pt).
Text text_skapa(SDL_Renderer *renderare, const char *typsnittsvag, int storlek);

// Uppdaterar texten som ska visas. Skapar ny texture internt.
bool text_satt(Text t, const char *innehall, SDL_Color farg);

// Ritar texten centrerad på (x, y).
void text_rita(Text t, int x, int y);

// Ritar texten med (x, y) som övre vänstra hörnet.
void text_rita_vid(Text t, int x, int y);

// Hämta bredd/höjd på den senast satta texten.
int text_hamta_bredd(Text t);
int text_hamta_hojd(Text t);

// Frigör allt minne och texturer.
void text_forstor(Text t);

#endif
