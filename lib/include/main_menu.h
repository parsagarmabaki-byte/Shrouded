#ifndef MAIN_MENU_H_INCLUDED
#define MAIN_MENU_H_INCLUDED

#include "lobby.h"

// Visar huvudmenyn med Start- och Exit-knappar.
// Återanvänder fönster, renderer och bakgrund från waitForPlayers.
//
// Returnerar:
//   1 om användaren tryckte Start (eller Enter)
//   0 om användaren tryckte Exit, Esc, eller stängde fönstret (SDL_QUIT)
int show_main_menu(waitForPlayers *pWait);

#endif