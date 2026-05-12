#pragma once

#include <SDL.h>
#include <SDL2/SDL_mixer.h>

#include <string>
#include <map>

class AudioManager {
private:
    // Background music
    std::map<std::string, Mix_Music*> BGMList;

    // Sound effects
    std::map<std::string, Mix_Chunk*> SFXList;

    // Track currently playing music
    std::string currentMusic = "";

    // Initialize audio systems
    bool audioInit();

public:
    // Constructor
    AudioManager(
        int startVolume = 64,
        std::string startingBGM = "rain"
    );

    // Destructor
    ~AudioManager();

    // Change overall volume
    void changeVolume(int volume = 64);

    // Swap looping background music
    void swapBGMusic(const std::string& newMusic);

    // Play sound effect
    void playSFX(const std::string& audio);

    // Get raw sound effect pointer
    Mix_Chunk* getSFX(const std::string& audioName);

    // Cleanup
    void audioClean();
};