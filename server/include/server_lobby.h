#ifndef SERVER_LOBBY_H
#define SERVER_LOBBY_H

#include <SDL2/SDL_net.h>
#include "network_data.h"

int get_player_id_from_sender(IPaddress *clientAddresses, int *clientUsed, IPaddress sender);
int add_to_lobby(GameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr);
int remove_from_lobby(GameState *state, IPaddress *clientAddresses, int *clientUsed, IPaddress addr);
void spawn_players(GameState *state);
int count_active_players(GameState *state);

#endif
