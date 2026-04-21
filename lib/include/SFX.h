#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    Mix_Chunk *button;
    Mix_Chunk *kill;
    Mix_Chunk *report;
    Mix_Music *start_game;

    // Music
    Mix_Music *lobby_music;
    Mix_Music *game_music;
} AudioAssets;

bool init_audio();
bool load_audio(AudioAssets *audio);
void cleanup_audio(AudioAssets *audio);

void play_lobby_music(Mix_Music *lobby_music, int loop);

void set_music_volume(int volume);
void stop_music();