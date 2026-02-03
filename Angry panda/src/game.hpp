#pragma once
#include <SDL.h>
#include "input.hpp"
#include "player.hpp"

class Game {
public:
    bool Init();
    void Run();
    void Shutdown();

private:
    void HandleEvents();
    void Update(float dt);
    void Render();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    bool running_ = false;

    InputState input_{};
    Player player_{};
};
