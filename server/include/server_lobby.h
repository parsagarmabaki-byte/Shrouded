#ifndef SERVER_LOBBY_H
#define SERVER_LOBBY_H

#include <SDL2/SDL_net.h>
#include "network_data.h"

int get_player_id_from_sender(IPaddress *clientAddresses, int *clientUsed, IPaddress sender);
int addToLobby(gameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr);
int removeFromLobby(gameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr);
void spawn_players(gameState *state);
int countActivePlayers(gameState *state);

#endif
