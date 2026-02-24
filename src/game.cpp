#include "game.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem> 

namespace fs = std::filesystem;

static const int kWindowWidth = 960;
static const int kWindowHeight = 540;

// Load BMP from a single path
static SDL_Texture* LoadTextureBMP(SDL_Renderer* renderer, const fs::path& path, int* out_w, int* out_h) {
    if (!fs::exists(path)) {
        std::cerr << "File not found: " << path << "\n";
        return nullptr;
    }

    SDL_Surface* bmp = SDL_LoadBMP(path.string().c_str());
    if (!bmp) {
        std::cerr << "Failed to load " << path << ": " << SDL_GetError() << "\n";
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, bmp);
    if (texture && out_w && out_h) {
        *out_w = bmp->w;
        *out_h = bmp->h;
    }

    SDL_FreeSurface(bmp);
    return texture;
}

bool Game::Init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    window_ = SDL_CreateWindow("Angry Panda",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               kWindowWidth, kWindowHeight,
                               SDL_WINDOW_SHOWN);
    if (!window_) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        return false;
    }

    // Determine the executable folder (build/)
    fs::path exe_dir = fs::current_path();

    // Load player base texture
    fs::path player_path = exe_dir / ".." / "assets" / "Opanda.bmp";
    player_texture_ = LoadTextureBMP(renderer_, player_path, &player_tex_w_, &player_tex_h_);
    if (!player_texture_) {
        std::cerr << "Failed to load " << player_path << "\n";
    } else {
        player_.SetTexture(player_texture_, player_tex_w_, player_tex_h_);
    }

    // Load idle frames (idel0.bmp → idel8.bmp)
    int idle_w = player_tex_w_;
    int idle_h = player_tex_h_;
    fs::path idle_dir = exe_dir / ".." / "assets" / "idel";
    for (int i = 0; i <= 8; ++i) {
        fs::path frame_path = idle_dir / ("idel" + std::to_string(i) + ".bmp");
        SDL_Texture* frame = LoadTextureBMP(renderer_, frame_path, &idle_w, &idle_h);
        if (frame) idle_textures_.push_back(frame);
    }

    // Load walk frames (walk1.bmp → walk10.bmp)
    int walk_w = player_tex_w_;
    int walk_h = player_tex_h_;
    fs::path walk_dir = exe_dir / ".." / "assets" / "walk";
    for (int i = 1; i <= 10; ++i) {
        fs::path frame_path = walk_dir / ("walk" + std::to_string(i) + ".bmp");
        SDL_Texture* frame = LoadTextureBMP(renderer_, frame_path, &walk_w, &walk_h);
        if (frame) walk_textures_.push_back(frame);
    }

    if (!idle_textures_.empty()) {
        player_.SetIdleTextures(idle_textures_, idle_w, idle_h);
        std::cout << "Loaded idle frames: " << idle_textures_.size() << "\n";
    } else {
        std::cerr << "No idle frames found in " << idle_dir << "\n";
    }

    if (!walk_textures_.empty()) {
        player_.SetWalkTextures(walk_textures_, walk_w, walk_h);
        std::cout << "Loaded walk frames: " << walk_textures_.size() << "\n";
    } else {
        std::cerr << "No walk frames found in " << walk_dir << "\n";
    }

    player_.SetGroundY(kWindowHeight - 80.0f);
    player_.SetPosition(120.0f, kWindowHeight - 80.0f);

    running_ = true;
    return true;
}
// Game loop
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

// Event handling
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

// Update player and game state
void Game::Update(float dt) {
    player_.Update(dt, input_);
}

// Render everything
void Game::Render() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer_, 25, 25, 30, 255);
    SDL_RenderClear(renderer_);

    // Draw ground
    SDL_SetRenderDrawColor(renderer_, 60, 60, 70, 255);
    SDL_Rect ground{0, kWindowHeight - 40, kWindowWidth, 40};
    SDL_RenderFillRect(renderer_, &ground);

    // Draw player
    player_.Render(renderer_);

    // Present final frame
    SDL_RenderPresent(renderer_);
}
void Game::Shutdown() {
    // Destroy idle textures
    for (SDL_Texture* tex : idle_textures_) {
        if (tex) SDL_DestroyTexture(tex);
    }
    idle_textures_.clear();

    // Destroy walk textures
    for (SDL_Texture* tex : walk_textures_) {
        if (tex) SDL_DestroyTexture(tex);
    }
    walk_textures_.clear();

    // Destroy player base texture
    if (player_texture_) {
        SDL_DestroyTexture(player_texture_);
        player_texture_ = nullptr;
    }

    // Destroy renderer and window
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    // Quit SDL
    SDL_Quit();
}