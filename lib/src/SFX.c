#include <stdio.h>
#include <string.h>
#include <SFX.h>

void zero_audio_assets(AudioAssets *audio)
{
    if (!audio){
        return;
    }
    memset(audio, 0, sizeof(AudioAssets));
}
bool init_audio()
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0){
        printf("Mix_OpenAudio: %s\n", Mix_GetError());
        return false;
    }

    Mix_AllocateChannels(16);

    return true;
}
bool load_audio(AudioAssets *audio)
{
    if (!audio){
        return false;
    }
    zero_audio_assets(audio);
    audio->lobby_music = Mix_LoadMUS("assets/SFX/lobby_music.mp3");
    if (!audio->lobby_music){
        printf("Failed to load lobby music: %s\n", Mix_GetError());
        return false;
    }

    // HÄR LÄGGER MAN TILL YTTERLIGARE LJUD ENLIGT FORMATET OVAN
    return true;
}
void cleanup_audio(AudioAssets *audio)
{
    if (!audio){
        return;
    }

    if (audio->lobby_music){
        Mix_FreeMusic(audio->lobby_music);
    }
    // HÄR RENSAR MAN LJUD ENLIGT FORMATET OVAN

    zero_audio_assets(audio);

    Mix_CloseAudio();
    
}

void play_lobby_music(Mix_Music *lobby_music, int loop)
{
    if (!lobby_music){
        return;
    }
    if (Mix_PlayMusic(lobby_music, loop) == -1){
        printf("Mix_PlayMusic failed: %s\n", Mix_GetError());
    }
}

void set_music_volume(int volume)
{
    if (volume < 0){
        volume = 0;
    }
    if (volume > 128){
        volume = 128;
    }
    Mix_VolumeMusic(volume);
}
void stop_music()
{
    Mix_HaltMusic();
}