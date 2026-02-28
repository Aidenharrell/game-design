#include "game.hpp"
#include <algorithm>
#include <cctype>
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

static fs::path ResolveAssetsDir() {
    fs::path exe_dir = fs::current_path();
    if (char* base = SDL_GetBasePath()) {
        exe_dir = fs::path(base);
        SDL_free(base);
    }

    const std::vector<fs::path> candidates = {
        exe_dir / "assets",
        exe_dir / ".." / "assets",
        fs::current_path() / "assets",
        fs::current_path() / ".." / "assets"
    };

    for (const fs::path& path : candidates) {
        if (fs::exists(path) && fs::is_directory(path)) {
            return path;
        }
    }
    return candidates[0];
}

static std::vector<fs::path> CollectFramesByPrefix(const fs::path& dir, const std::string& prefix) {
    std::vector<fs::path> frames;
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return frames;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;

        const fs::path path = entry.path();
        std::string stem = path.stem().string();
        std::string ext = path.extension().string();
        std::transform(stem.begin(), stem.end(), stem.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (ext == ".bmp" && stem.rfind(prefix, 0) == 0) {
            frames.push_back(path);
        }
    }

    auto frame_index = [&](const fs::path& p) -> int {
        const std::string stem = p.stem().string();
        std::size_t i = prefix.size();
        int value = 0;
        bool has_digits = false;
        while (i < stem.size() && std::isdigit(static_cast<unsigned char>(stem[i]))) {
            has_digits = true;
            value = (value * 10) + (stem[i] - '0');
            ++i;
        }
        return has_digits ? value : -1;
    };

    std::sort(frames.begin(), frames.end(), [&](const fs::path& a, const fs::path& b) {
        const int ai = frame_index(a);
        const int bi = frame_index(b);
        if (ai != bi) return ai < bi;
        return a.filename().string() < b.filename().string();
    });
    return frames;
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

    fs::path assets_dir = ResolveAssetsDir();

    // Load player base texture
    fs::path player_path = assets_dir / "Opanda.bmp";
    player_texture_ = LoadTextureBMP(renderer_, player_path, &player_tex_w_, &player_tex_h_);
    if (!player_texture_) {
        std::cerr << "Failed to load " << player_path << "\n";
    } else {
        player_.SetTexture(player_texture_, player_tex_w_, player_tex_h_);
    }

    // Load idle frames by filename prefix (idle*.bmp or idel*.bmp)
    int idle_w = player_tex_w_;
    int idle_h = player_tex_h_;
    fs::path idle_dir = assets_dir / "idel";
    std::vector<fs::path> idle_frames = CollectFramesByPrefix(idle_dir, "idle");
    if (idle_frames.empty()) {
        idle_frames = CollectFramesByPrefix(idle_dir, "idel");
    }
    if (idle_frames.empty()) {
        idle_frames = CollectFramesByPrefix(assets_dir, "idle");
    }
    if (idle_frames.empty()) {
        idle_frames = CollectFramesByPrefix(assets_dir, "idel");
    }
    for (const fs::path& frame_path : idle_frames) {
        SDL_Texture* frame = LoadTextureBMP(renderer_, frame_path, &idle_w, &idle_h);
        if (frame) idle_textures_.push_back(frame);
    }

    // Load walk frames from either walk*.bmp or rewalk*.bmp
    int walk_w = player_tex_w_;
    int walk_h = player_tex_h_;
    fs::path walk_dir = assets_dir / "walk";
    std::vector<fs::path> walk_frames = CollectFramesByPrefix(walk_dir, "walk");
    if (walk_frames.empty()) {
        walk_frames = CollectFramesByPrefix(walk_dir, "rewalk");
    }
    if (walk_frames.empty()) {
        walk_frames = CollectFramesByPrefix(assets_dir, "walk");
    }
    if (walk_frames.empty()) {
        walk_frames = CollectFramesByPrefix(assets_dir, "rewalk");
    }
    for (const fs::path& frame_path : walk_frames) {
        SDL_Texture* frame = LoadTextureBMP(renderer_, frame_path, &walk_w, &walk_h);
        if (frame) walk_textures_.push_back(frame);
    }

    // Load punch frames from either punch*.bmp or repunch*.bmp
    int punch_w = player_tex_w_;
    int punch_h = player_tex_h_;
    fs::path punch_dir = assets_dir / "punch";
    std::vector<fs::path> punch_frames = CollectFramesByPrefix(punch_dir, "punch");
    if (punch_frames.empty()) {
        punch_frames = CollectFramesByPrefix(punch_dir, "repunch");
    }
    if (punch_frames.empty()) {
        punch_frames = CollectFramesByPrefix(assets_dir, "punch");
    }
    if (punch_frames.empty()) {
        punch_frames = CollectFramesByPrefix(assets_dir, "repunch");
    }
    for (const fs::path& frame_path : punch_frames) {
        SDL_Texture* frame = LoadTextureBMP(renderer_, frame_path, &punch_w, &punch_h);
        if (frame) punch_textures_.push_back(frame);
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
    if (!punch_textures_.empty()) {
        player_.SetPunchTextures(punch_textures_, punch_w, punch_h);
        std::cout << "Loaded punch frames: " << punch_textures_.size() << "\n";
    } else {
        std::cerr << "No punch frames found in " << punch_dir << "\n";
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
    // Destroy punch textures
    for (SDL_Texture* tex : punch_textures_) {
        if (tex) SDL_DestroyTexture(tex);
    }
    punch_textures_.clear();

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
