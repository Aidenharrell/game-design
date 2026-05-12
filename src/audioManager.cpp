#include "audioManager.hpp"

#include <iostream>


AudioManager::AudioManager(
    int startVolume,
    std::string startingBGM
) {
    audioInit();

    // Background music
    BGMList["rain"] = Mix_LoadMUS("../assets/sounds/rainsound.wav");

    // Sound effects
    SFXList["walk"]  = Mix_LoadWAV("../assets/sounds/cartoonwalk.wav");
    SFXList["hurt"]  = Mix_LoadWAV("../assets/sounds/hurt.wav");
    SFXList["death"] = Mix_LoadWAV("../assets/sounds/Death.wav");
    SFXList["punch"] = Mix_LoadWAV("../assets/sounds/Punch.wav");
    SFXList["heel"]  = Mix_LoadWAV("../assets/sounds/heel.wav");

    changeVolume(startVolume);

    currentMusic = startingBGM;

    if (BGMList.count(startingBGM)) {
        Mix_PlayMusic(BGMList[startingBGM], -1);
    }
}


AudioManager::~AudioManager() {
    audioClean();
}


bool AudioManager::audioInit() {
    // SDL audio subsystem
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr
            << "SDL audio init failed: "
            << SDL_GetError()
            << "\n";
        return false;
    }

    // Mixer
    if (Mix_OpenAudio(
            44100,
            MIX_DEFAULT_FORMAT,
            2,
            2048
        ) < 0) {
        std::cerr
            << "Mixer init failed: "
            << Mix_GetError()
            << "\n";
        return false;
    }

    Mix_AllocateChannels(32);

    return true;
}


void AudioManager::changeVolume(int volume) {
    Mix_VolumeMusic(volume);

    for (auto& [name, sound] : SFXList) {
        Mix_VolumeChunk(sound, volume);
    }
}


void AudioManager::swapBGMusic(const std::string& newMusic) {
    if (currentMusic == newMusic) {
        return;
    }

    if (!BGMList.count(newMusic)) {
        std::cerr
            << "Music not found: "
            << newMusic
            << "\n";
        return;
    }

    currentMusic = newMusic;

    Mix_FadeOutMusic(1000);
    Mix_FadeInMusic(BGMList[newMusic], -1, 1000);
}


void AudioManager::playSFX(const std::string& audio) {
    if (!SFXList.count(audio)) {
        std::cerr
            << "Sound not found: "
            << audio
            << "\n";
        return;
    }

    Mix_PlayChannel(-1, SFXList[audio], 0);
}


Mix_Chunk* AudioManager::getSFX(const std::string& audioName) {
    if (!SFXList.count(audioName)) {
        return nullptr;
    }

    return SFXList[audioName];
}


void AudioManager::audioClean() {
    for (auto& [name, music] : BGMList) {
        Mix_FreeMusic(music);
    }

    for (auto& [name, sound] : SFXList) {
        Mix_FreeChunk(sound);
    }

    BGMList.clear();
    SFXList.clear();

    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}