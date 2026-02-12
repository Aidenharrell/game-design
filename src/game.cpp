#include "game.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

static const int kWindowWidth = 960;
static const int kWindowHeight = 540;

static SDL_Texture* LoadTextureBMP(SDL_Renderer* renderer, const std::vector<std::string>& paths, int* out_w, int* out_h) {
    for (const std::string& path : paths) {
        SDL_Surface* bmp = SDL_LoadBMP(path.c_str());
        if (!bmp) {
            continue;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, bmp);
        if (texture && out_w && out_h) {
            *out_w = bmp->w;
            *out_h = bmp->h;
        }
        SDL_FreeSurface(bmp);
        if (texture) {
            return texture;
        }
    }
    return nullptr;
}

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

    player_texture_ = LoadTextureBMP(
        renderer_,
        {
            "src/Opanda.bmp",
            "..\\..\\src\\Opanda.bmp",
            "C:\\Users\\aiden\\game-design\\src\\Opanda.bmp"
        },
        &player_tex_w_,
        &player_tex_h_
    );

    if (player_texture_) {
        player_.SetTexture(player_texture_, player_tex_w_, player_tex_h_);
    } else {
        std::cerr << "Failed to load Opanda.bmp\n";
    }

    int idle_w = player_tex_w_;
    int idle_h = player_tex_h_;
    for (int i = 0; i < 8; ++i) {
        const std::string filename = "idel" + std::to_string(i) + ".bmp";
        SDL_Texture* frame = LoadTextureBMP(
            renderer_,
            {
                "src/panda idel/" + filename,
                "..\\..\\src\\panda idel\\" + filename,
                "..\\..\\..\\OneDrive\\Desktop\\pixelart\\panda idel\\" + filename,
                "C:\\Users\\aiden\\OneDrive\\Desktop\\pixelart\\panda idel\\" + filename
            },
            &idle_w,
            &idle_h
        );
        if (frame) {
            idle_textures_.push_back(frame);
        }
    }

    if (!idle_textures_.empty()) {
        player_.SetIdleTextures(idle_textures_, idle_w, idle_h);
        std::cout << "Loaded idle frames: " << idle_textures_.size() << "\n";
    } else {
        std::cerr << "No idle frames found in panda idel folder\n";
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
    for (SDL_Texture* tex : idle_textures_) {
        if (tex) {
            SDL_DestroyTexture(tex);
        }
    }
    idle_textures_.clear();

    if (player_texture_) {
        SDL_DestroyTexture(player_texture_);
        player_texture_ = nullptr;
    }
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

    SDL_SetRenderDrawColor(renderer_, 60, 60, 70, 255);
    SDL_Rect ground{0, kWindowHeight - 40, kWindowWidth, 40};
    SDL_RenderFillRect(renderer_, &ground);

    player_.Render(renderer_);

    SDL_RenderPresent(renderer_);
}
