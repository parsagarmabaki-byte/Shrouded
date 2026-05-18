#ifndef SFX_H
#define SFX_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    Mix_Chunk *button;
    Mix_Chunk *kill_knife;
    Mix_Chunk *dramatic_kill;
    Mix_Chunk *meeting_horn;
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
void play_kill_knife(AudioAssets *audio);
void play_dramatic_kill(AudioAssets *audio);
void play_meeting_horn(AudioAssets *audio);

void set_music_volume(int volume);
void stop_music();

#endif
