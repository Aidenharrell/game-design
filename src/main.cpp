#include "game.hpp"
#include <SDL.h>
#include <iostream>

namespace {
// Holds the loaded WAV data and tracks where playback currently is.
// SDL's callback needs this state so it knows what bytes to copy next.
struct LoopingAudio {
    Uint8* data = nullptr;
    Uint32 length = 0;
    Uint32 position = 0;
};

// SDL calls this function whenever the audio device needs more sound data.
// This version loops the rain file forever by resetting to the start.
void AudioCallback(void* userdata, Uint8* stream, int len) {
    LoopingAudio* audio = static_cast<LoopingAudio*>(userdata);
    SDL_memset(stream, 0, len);

    if (!audio || !audio->data || audio->length == 0) {
        return;
    }

    int mixed = 0;
    while (mixed < len) {
        if (audio->position >= audio->length) {
            audio->position = 0;
        }
        // counter showing how long music has left before reset.
        const Uint32 remaining = audio->length - audio->position;
        const int chunk = static_cast<int>(remaining < static_cast<Uint32>(len - mixed)
            ? remaining
            : static_cast<Uint32>(len - mixed));

        SDL_MixAudioFormat(stream + mixed,
                           audio->data + audio->position,
                           AUDIO_S16LSB,
                           static_cast<Uint32>(chunk),
                           SDL_MIX_MAXVOLUME / 1);
        audio->position += static_cast<Uint32>(chunk);
        mixed += chunk;
    }
}



bool LoadLoopingRain(LoopingAudio* audio, SDL_AudioSpec* device_spec) {
    if (!audio || !device_spec) {
        return false;
    }

    // This uses a plain relative path instead of building it with filesystem.
    const char* rain_path = "assets/sounds/rainsound.wav";
    SDL_AudioSpec source_spec{};
    Uint8* source_buffer = nullptr;
    Uint32 source_length = 0;
    if (!SDL_LoadWAV(rain_path, &source_spec, &source_buffer, &source_length)) {
        std::cerr << "Failed to load " << rain_path << ": " << SDL_GetError() << "\n";
        return false;
    }

    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt,
                          source_spec.format, source_spec.channels, source_spec.freq,
                          device_spec->format, device_spec->channels, device_spec->freq) < 0) {
        std::cerr << "Failed to build audio converter: " << SDL_GetError() << "\n";
        SDL_FreeWAV(source_buffer);
        return false;
    }

    cvt.len = static_cast<int>(source_length);
    cvt.buf = static_cast<Uint8*>(SDL_malloc(cvt.len * cvt.len_mult));
    if (!cvt.buf) {
        std::cerr << "Failed to allocate audio buffer.\n";
        SDL_FreeWAV(source_buffer);
        return false;
    }

    SDL_memcpy(cvt.buf, source_buffer, source_length);
    SDL_FreeWAV(source_buffer);

    if (SDL_ConvertAudio(&cvt) < 0) {
        std::cerr << "Failed to convert audio: " << SDL_GetError() << "\n";
        SDL_free(cvt.buf);
        return false;
    }

    audio->data = cvt.buf;
    audio->length = static_cast<Uint32>(cvt.len_cvt);
    audio->position = 0;
    return true;
}
} 

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Starts SDL audio before we try to open an audio device.
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL audio init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    LoopingAudio rain_audio{};
    SDL_AudioSpec desired{};
    desired.freq = 44100;
    desired.format = AUDIO_S16LSB;
    desired.channels = 2;
    desired.samples = 2048;

    // Tells SDL which function fills the speaker output and what data to pass
    // into that function.
    desired.callback = AudioCallback;
    desired.userdata = &rain_audio;

    SDL_AudioSpec obtained{};

    // Opens the actual audio device. Without this, SDL has nowhere to play.
    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (audio_device == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << "\n";
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return 1;
    }

    // Load the rain file, then unpause the device so playback starts.
    if (LoadLoopingRain(&rain_audio, &obtained)) {
        SDL_PauseAudioDevice(audio_device, 0);
    }

    Game game;
    const bool initialized = game.Init();
    if (initialized) {
        game.Run();
        game.Shutdown();
    }

    // Always clean up the device and the allocated WAV buffer.
    SDL_CloseAudioDevice(audio_device);
    SDL_free(rain_audio.data);

    // Shuts down only the audio subsystem we started at the top.
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return initialized ? 0 : 1;
}
