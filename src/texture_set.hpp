#pragma once

#include <SDL.h>
#include <vector>

struct TextureSet {
    std::vector<SDL_Texture*> frames{};
    int width = 0;
    int height = 0;

    bool Empty() const { return frames.empty(); }
    SDL_Texture* First() const { return frames.empty() ? nullptr : frames.front(); }
};
