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

    audio->kill_knife = Mix_LoadWAV("assets/SFX/kill_knife.mp3");
    if (!audio->kill_knife){
        printf("Failed to load knife_kill_effect: %s\n", Mix_GetError());
        return false;
    }
    Mix_VolumeChunk(audio->kill_knife, MIX_MAX_VOLUME / 2);

    audio->dramatic_kill = Mix_LoadWAV("assets/SFX/dramatic_kill.mp3");
    if (!audio->dramatic_kill){
        printf("Failed to load dramatic_kill_effect: %s\n", Mix_GetError());
        return false;
    }
    Mix_VolumeChunk(audio->dramatic_kill, MIX_MAX_VOLUME / 3);

    audio->meeting_horn = Mix_LoadWAV("assets/SFX/meeting_horn.mp3");
    if (!audio->meeting_horn){
        printf("Failed to load meeting_horn_effect: %s\n", Mix_GetError());
        return false;
    }
    Mix_VolumeChunk(audio->meeting_horn, MIX_MAX_VOLUME / 2);

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
    if (audio->kill_knife){
        Mix_FreeChunk(audio->kill_knife);
    }
    if (audio->dramatic_kill){
        Mix_FreeChunk(audio->dramatic_kill);
    }
    if (audio->meeting_horn){
        Mix_FreeChunk(audio->meeting_horn);
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

void play_kill_knife(AudioAssets *audio)
{
    if (!audio || !audio->kill_knife){
        return;
    }
    if (Mix_PlayChannel(-1, audio->kill_knife, 0) == -1){
        printf("Mix_PlayChannel failed: %s\n", Mix_GetError());
    }
}

void play_dramatic_kill(AudioAssets *audio)
{
    if (!audio || !audio->dramatic_kill){
        return;
    }
    if (Mix_PlayChannel(-1, audio->dramatic_kill, 0) == -1){
        printf("Mix_PlayChannel failed: %s\n", Mix_GetError());
    }
}

void play_meeting_horn(AudioAssets *audio)
{
    if (!audio || !audio->meeting_horn){
        return;
    }
    if (Mix_PlayChannel(-1, audio->meeting_horn, 0) == -1){
        printf("Mix_PlayChannel failed: %s\n", Mix_GetError());
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
