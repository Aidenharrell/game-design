#include "game.hpp"
#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

namespace {
struct LoopingAudio {
    Uint8* data = nullptr;
    Uint32 length = 0;
    Uint32 position = 0;
    SDL_AudioFormat format = AUDIO_S16LSB;
    int volume = SDL_MIX_MAXVOLUME / 4;
};

fs::path ResolveRainPath() {
    fs::path exe_dir = fs::current_path();
    if (char* base = SDL_GetBasePath()) {
        exe_dir = fs::path(base);
        SDL_free(base);
    }

    const std::vector<fs::path> candidates = {
        exe_dir / "assets" / "sounds" / "rainsound.wav",
        exe_dir / ".." / "assets" / "sounds" / "rainsound.wav",
        fs::current_path() / "assets" / "sounds" / "rainsound.wav",
        fs::current_path() / ".." / "assets" / "sounds" / "rainsound.wav"
    };

    for (const fs::path& path : candidates) {
        if (fs::exists(path) && fs::is_regular_file(path)) {
            return path;
        }
    }

    return candidates[0];
}

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

        const Uint32 remaining = audio->length - audio->position;
        const int chunk = static_cast<int>(remaining < static_cast<Uint32>(len - mixed)
            ? remaining
            : static_cast<Uint32>(len - mixed));

        SDL_MixAudioFormat(stream + mixed,
                           audio->data + audio->position,
                           audio->format,
                           static_cast<Uint32>(chunk),
                           audio->volume);
        audio->position += static_cast<Uint32>(chunk);
        mixed += chunk;
    }
}

bool LoadLoopingRain(LoopingAudio* audio, SDL_AudioSpec* device_spec) {
    if (!audio || !device_spec) {
        return false;
    }

    const fs::path rain_path = ResolveRainPath();
    SDL_AudioSpec source_spec{};
    Uint8* source_buffer = nullptr;
    Uint32 source_length = 0;
    if (!SDL_LoadWAV(rain_path.string().c_str(), &source_spec, &source_buffer, &source_length)) {
        std::cerr << "Warning: failed to load " << rain_path << ": " << SDL_GetError() << "\n";
        return false;
    }

    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt,
                          source_spec.format, source_spec.channels, source_spec.freq,
                          device_spec->format, device_spec->channels, device_spec->freq) < 0) {
        std::cerr << "Warning: failed to build audio converter: " << SDL_GetError() << "\n";
        SDL_FreeWAV(source_buffer);
        return false;
    }

    cvt.len = static_cast<int>(source_length);
    cvt.buf = static_cast<Uint8*>(SDL_malloc(cvt.len * cvt.len_mult));
    if (!cvt.buf) {
        std::cerr << "Warning: failed to allocate audio buffer.\n";
        SDL_FreeWAV(source_buffer);
        return false;
    }

    SDL_memcpy(cvt.buf, source_buffer, source_length);
    SDL_FreeWAV(source_buffer);

    if (SDL_ConvertAudio(&cvt) < 0) {
        std::cerr << "Warning: failed to convert audio: " << SDL_GetError() << "\n";
        SDL_free(cvt.buf);
        return false;
    }

    audio->data = cvt.buf;
    audio->length = static_cast<Uint32>(cvt.len_cvt);
    audio->position = 0;
    audio->format = device_spec->format;
    audio->volume = SDL_MIX_MAXVOLUME / 4;
    return true;
}
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    LoopingAudio rain_audio{};
    SDL_AudioDeviceID audio_device = 0;

    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        std::cerr << "Warning: SDL audio init failed: " << SDL_GetError() << "\n";
    } else {
        SDL_AudioSpec desired{};
        desired.freq = 44100;
        desired.format = AUDIO_S16LSB;
        desired.channels = 2;
        desired.samples = 4096;
        desired.callback = AudioCallback;
        desired.userdata = &rain_audio;

        SDL_AudioSpec obtained{};
        audio_device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
        if (audio_device == 0) {
            std::cerr << "Warning: failed to open audio device: " << SDL_GetError() << "\n";
        } else if (LoadLoopingRain(&rain_audio, &obtained)) {
            SDL_PauseAudioDevice(audio_device, 0);
        }
    }

    Game game;
    const bool initialized = game.Init();
    if (initialized) {
        game.Run();
    }

    if (audio_device != 0) {
        SDL_CloseAudioDevice(audio_device);
    }
    SDL_free(rain_audio.data);
    game.Shutdown();
    return initialized ? 0 : 1;
}
