#include "game.hpp"
#include <algorithm>
#include <iostream>

static const int kWindowWidth = 960;
static const int kWindowHeight = 540;

bool Game::Init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    window_ = SDL_CreateWindow(
        "Angry Panda",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        kWindowWidth, kWindowHeight,
        SDL_WINDOW_SHOWN
    );
    if (!window_) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        return false;
    }

    player_.SetGroundY(kWindowHeight - 80.0f);
    player_.SetPosition(120.0f, kWindowHeight - 80.0f);

    running_ = true;
    return true;
}

void Game::Run() {
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 last = now;

    while (running_) {
        last = now;
        now = SDL_GetPerformanceCounter();
        double freq = static_cast<double>(SDL_GetPerformanceFrequency());
        float dt = static_cast<float>((now - last) / freq);

        HandleEvents();
        Update(dt);
        Render();
    }
}

void Game::Shutdown() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void Game::HandleEvents() {
    SDL_Event e;
    input_.ClearFrame();

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running_ = false;
        } else if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            input_.OnKeyDown(e.key.keysym.sym);
        } else if (e.type == SDL_KEYUP) {
            input_.OnKeyUp(e.key.keysym.sym);
        }
    }
}

void Game::Update(float dt) {
    player_.Update(dt, input_);
}

void Game::Render() {
    SDL_SetRenderDrawColor(renderer_, 25, 25, 30, 255);
    SDL_RenderClear(renderer_);

    // Ground
    SDL_SetRenderDrawColor(renderer_, 60, 60, 70, 255);
    SDL_Rect ground{0, kWindowHeight - 40, kWindowWidth, 40};
    SDL_RenderFillRect(renderer_, &ground);

    // Player
    player_.Render(renderer_);

    SDL_RenderPresent(renderer_);
}
