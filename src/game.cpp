#include "game.hpp"
#include "platform.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <SDL.h> 

namespace fs = std::filesystem;

static const int kWindowWidth = 960;
static const int kWindowHeight = 540;

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

static TextureSet LoadSingleTexture(SDL_Renderer* renderer, const fs::path& path) {
    TextureSet texture_set;
    SDL_Texture* texture = LoadTextureBMP(renderer, path, &texture_set.width, &texture_set.height);
    if (texture) {
        texture_set.frames.push_back(texture);
    }
    return texture_set;
}

static TextureSet LoadSingleTexture(SDL_Renderer* renderer,
                                    const std::vector<fs::path>& candidate_paths,
                                    fs::path* out_loaded_path = nullptr) {
    for (const fs::path& candidate : candidate_paths) {
        TextureSet texture_set = LoadSingleTexture(renderer, candidate);
        if (!texture_set.Empty()) {
            if (out_loaded_path) {
                *out_loaded_path = candidate;
            }
            return texture_set;
        }
    }
    return {};
}

static TextureSet LoadTextureSet(SDL_Renderer* renderer, const std::vector<fs::path>& frame_paths) {
    TextureSet texture_set;
    for (const fs::path& frame_path : frame_paths) {
        int frame_w = 0;
        int frame_h = 0;
        SDL_Texture* frame = LoadTextureBMP(renderer, frame_path, &frame_w, &frame_h);
        if (!frame) {
            continue;
        }

        if (texture_set.Empty()) {
            texture_set.width = frame_w;
            texture_set.height = frame_h;
        }
        texture_set.frames.push_back(frame);
    }
    return texture_set;
}

static void DestroyTextureSet(TextureSet& texture_set) {
    for (SDL_Texture* texture : texture_set.frames) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
    texture_set.frames.clear();
    texture_set.width = 0;
    texture_set.height = 0;
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

static std::vector<fs::path> CollectFramesByPrefix(const std::vector<fs::path>& dirs, const std::string& prefix) {
    for (const fs::path& dir : dirs) {
        std::vector<fs::path> frames = CollectFramesByPrefix(dir, prefix);
        if (!frames.empty()) {
            return frames;
        }
    }
    return {};
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

    fs::path player_path = assets_dir / "Opanda.bmp";
    player_texture_ = LoadSingleTexture(renderer_, player_path);
    fs::path platform_path = assets_dir / "branch.bmp";
    platform_textures_ = LoadSingleTexture(renderer_, platform_path);
    fs::path background_path = assets_dir / "Background.bmp";
    background_texture_ = LoadSingleTexture(renderer_, background_path);
    fs::path tree_path = assets_dir / "tree.bmp";
    tree_texture_ = LoadSingleTexture(renderer_, tree_path);
    fs::path bush_path = assets_dir / "bush.bmp";
    bush_texture_ = LoadSingleTexture(renderer_, bush_path);
    if (player_texture_.Empty()) {
        std::cerr << "Failed to load " << player_path << "\n";
    } else {
        player_.SetTexture(player_texture_);
    }

    fs::path idle_dir = assets_dir / "idel";
    std::vector<fs::path> idle_frames = CollectFramesByPrefix(idle_dir, "idel");
    if (idle_frames.empty()) {
        idle_frames = CollectFramesByPrefix(idle_dir, "idel");
    }
    if (idle_frames.empty()) {
        idle_frames = CollectFramesByPrefix(assets_dir, "idel");
    }
    idle_textures_ = LoadTextureSet(renderer_, idle_frames);

    fs::path walk_dir = assets_dir / "walk";
    std::vector<fs::path> walk_frames = CollectFramesByPrefix(walk_dir, "walk");
    if (walk_frames.empty()) {
        walk_frames = CollectFramesByPrefix(walk_dir, "rewalk");
    }
    if (walk_frames.empty()) {
        walk_frames = CollectFramesByPrefix(assets_dir, "rewalk");
    }
    walk_textures_ = LoadTextureSet(renderer_, walk_frames);

    fs::path jump_dir = assets_dir / "jump";
    std::vector<fs::path> jump_frames = CollectFramesByPrefix(jump_dir, "jump");
    if (jump_frames.empty()) {
        jump_frames = CollectFramesByPrefix(assets_dir, "jump");
    }
    jump_textures_ = LoadTextureSet(renderer_, jump_frames);

    fs::path punch_dir = assets_dir / "punch";
    std::vector<fs::path> punch_frames = CollectFramesByPrefix(punch_dir, "punch");
    if (punch_frames.empty()) {
        punch_frames = CollectFramesByPrefix(assets_dir, "punch");
    }
    punch_textures_ = LoadTextureSet(renderer_, punch_frames);

    fs::path heel_kick_dir = assets_dir / "heel";
    std::vector<fs::path> heel_kick_frames = CollectFramesByPrefix(heel_kick_dir, "heel");
    if (heel_kick_frames.empty()) {
        heel_kick_frames = CollectFramesByPrefix(assets_dir, "heel");
    }
    heel_kick_textures_ = LoadTextureSet(renderer_, heel_kick_frames);

    fs::path background_path;
    background_texture_ = LoadSingleTexture(
        renderer_,
        {
            assets_dir / "background" / "Background.bmp",
            assets_dir / "Background.bmp"
        },
        &background_path);

    fs::path tree_path;
    tree_texture_ = LoadSingleTexture(
        renderer_,
        {
            assets_dir / "tree" / "tree.bmp",
            assets_dir / "tree.bmp"
        },
        &tree_path);

    fs::path bush_path;
    bush_texture_ = LoadSingleTexture(
        renderer_,
        {
            assets_dir / "tree" / "bush.bmp",
            assets_dir / "bush.bmp"
        },
        &bush_path);

    fs::path branch_path;
    platform_textures_ = LoadSingleTexture(
        renderer_,
        {
            assets_dir / "tree" / "branch.bmp",
            assets_dir / "branch.bmp"
        },
        &branch_path);

    std::vector<fs::path> squirrel_frames = CollectFramesByPrefix(
        std::vector<fs::path>{
            assets_dir / "squirrelshot",
            assets_dir
        },
        "shoot");
    squirrel_textures_ = LoadTextureSet(renderer_, squirrel_frames);

    std::vector<fs::path> acorn_frames = CollectFramesByPrefix(
        std::vector<fs::path>{
            assets_dir / "acorn",
            assets_dir
        },
        "acorn");
    acorn_textures_ = LoadTextureSet(renderer_, acorn_frames);

    if (!idle_textures_.Empty()) {
        player_.SetIdleTextures(idle_textures_);
        std::cout << "Loaded idle frames: " << idle_textures_.frames.size() << "\n";
    } else {
        std::cerr << "No idle frames found in " << idle_dir << "\n";
    }

    if (!walk_textures_.Empty()) {
        player_.SetWalkTextures(walk_textures_);
        std::cout << "Loaded walk frames: " << walk_textures_.frames.size() << "\n";
    } else {
        std::cerr << "No walk frames found in " << walk_dir << "\n";
    }
    if (!jump_textures_.Empty()) {
        player_.SetJumpTextures(jump_textures_);
        std::cout << "Loaded jump frames: " << jump_textures_.frames.size() << "\n";
    } else {
        std::cerr << "No jump frames found in " << jump_dir << "\n";}
    if (!punch_textures_.Empty()) {
        player_.SetPunchTextures(punch_textures_);
        std::cout << "Loaded punch frames: " << punch_textures_.frames.size() << "\n";
    } else {
        std::cerr << "No punch frames found in " << punch_dir << "\n";
    }
    if (!heel_kick_textures_.Empty()) {
        player_.SetHeelKickTextures(heel_kick_textures_);
        std::cout << "Loaded heel kick frames: " << heel_kick_textures_.frames.size() << "\n";
    } else {
        std::cerr << "No heel kick frames found in " << heel_kick_dir << " (or flipkick prefix)\n";
    }
    if (background_texture_.Empty()) {
        std::cerr << "No background texture found under " << assets_dir << "\n";
    }
    if (tree_texture_.Empty()) {
        std::cerr << "No tree texture found under " << assets_dir << "\n";
    }
    if (bush_texture_.Empty()) {
        std::cerr << "No bush texture found under " << assets_dir << "\n";
    }
    if (platform_textures_.Empty()) {
        std::cerr << "No branch texture found under " << assets_dir << "\n";
    }
    if (squirrel_textures_.Empty()) {
        std::cerr << "No squirrel frames found under " << assets_dir << "\n";
    }
    if (acorn_textures_.Empty()) {
        std::cerr << "No acorn frames found under " << assets_dir << "\n";
    }

    player_.SetGroundY(kWindowHeight - 40.0f);
    player_.SetPosition(120.0f, kWindowHeight - 80.0f);

    //platforms
    platforms_.push_back({ SDL_Rect{0, kWindowHeight - 40, 5000, 40} });  // ground

    platforms_.push_back({ SDL_Rect{300, 400, 200, 50} });
    platforms_.push_back({ SDL_Rect{600, 300, 200, 50} });
    platforms_.push_back({ SDL_Rect{300, 250, 200, 50} });
    platforms_.push_back({ SDL_Rect{600, 150, 200, 50} });

    SquirrelEnemy lower_squirrel;
    lower_squirrel.SetPosition(360.0f, 400.0f);
    lower_squirrel.SetTextures(squirrel_textures_, acorn_textures_);
    squirrels_.push_back(lower_squirrel);

    SquirrelEnemy upper_squirrel;
    upper_squirrel.SetPosition(640.0f, 150.0f);
    upper_squirrel.SetTextures(squirrel_textures_, acorn_textures_);
    squirrels_.push_back(upper_squirrel);

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
    player_.CheckPlatformCollisions(platforms_);

    const SDL_Rect player_rect = player_.GetBodyRect();
    const SDL_Rect attack_rect = player_.GetAttackRect();

    for (SquirrelEnemy& squirrel : squirrels_) {
        squirrel.Update(dt, player_rect);

        if (attack_rect.w > 0 && attack_rect.h > 0) {
            squirrel.TryTakeHit(attack_rect);
        }

        float knockback_x = 0.0f;
        if (squirrel.CheckProjectileHitPlayer(player_rect, &knockback_x)) {
            player_.ApplyKnockback(knockback_x, -220.0f);
        }
    }

    camera_x_ = player_.GetX() - 480;
}
// Render everything
void Game::Render() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer_, 25, 25, 30, 255);
    SDL_RenderClear(renderer_);

    // Draw background
    if(!background_texture_.Empty())
    {
        SDL_Rect bgRect;
        bgRect.x = 0;
        bgRect.y = 0;
        bgRect.w = kWindowWidth;
        bgRect.h = kWindowHeight;
        SDL_RenderCopy(renderer_, background_texture_.frames[0], NULL, &bgRect);
    }

    // Draw trees
    if(!tree_texture_.Empty())
    {
        SDL_Rect treeRect;
        treeRect.x = 150 - static_cast<int>(camera_x_); // left/right
        treeRect.y = 0;
        treeRect.w = 220; // wider/narrower
        treeRect.h = kWindowHeight;
        SDL_RenderCopy(renderer_, tree_texture_.frames[0], NULL, &treeRect);
    }
    if(!tree_texture_.Empty())
    {
        SDL_Rect treeRect;
        treeRect.x = 450 - static_cast<int>(camera_x_); // left/right
        treeRect.y = 0;
        treeRect.w = 225; // wider/narrower
        treeRect.h = kWindowHeight;
        SDL_RenderCopy(renderer_, tree_texture_.frames[0], NULL, &treeRect);
    }

    // Draw ground
    SDL_SetRenderDrawColor(renderer_, 34, 139, 34, 255);
    SDL_Rect ground{0, kWindowHeight - 40, kWindowWidth, 40};
    SDL_RenderFillRect(renderer_, &ground);

    //Draw platforms
    for(const auto& platform : platforms_)
    {
        if(platform.rect.w > 1000) continue;
        SDL_Rect screenRect;
        screenRect.w = platform.rect.w;
        screenRect.h = platform.rect.h;
        screenRect.x = platform.rect.x - static_cast<int>(camera_x_);
        screenRect.y = platform.rect.y;
        if(!platform_textures_.Empty())
        {
            SDL_RenderCopy(renderer_, platform_textures_.frames[0], NULL, &screenRect);
        }
    }

    for (const SquirrelEnemy& squirrel : squirrels_) {
        squirrel.Render(renderer_, camera_x_);
    }

    // Draw player
    player_.Render(renderer_, camera_x_);

    // Draw bushes NEW
    if(!bush_texture_.Empty())
    {
        for(int i = 0; i < 20; i++) // number of bushes
        {
            int xPos = i * 300; // spacing

            SDL_Rect bushRect;
            bushRect.x = xPos - static_cast<int>(camera_x_);
            bushRect.y = kWindowHeight - 100; // height
            bushRect.w = 70; // size
            bushRect.h = 80;
            SDL_RenderCopy(renderer_, bush_texture_.frames[0], NULL, &bushRect);
        }
    }

    // Present final frame
    SDL_RenderPresent(renderer_);
}
void Game::Shutdown() {
    DestroyTextureSet(idle_textures_);
    DestroyTextureSet(walk_textures_);
    DestroyTextureSet(jump_textures_);
    DestroyTextureSet(punch_textures_);
    DestroyTextureSet(heel_kick_textures_);
    DestroyTextureSet(player_texture_);
    DestroyTextureSet(platform_textures_);
    DestroyTextureSet(ground_texture_);
    DestroyTextureSet(background_texture_);
    DestroyTextureSet(tree_texture_);
    DestroyTextureSet(bush_texture_);
    DestroyTextureSet(squirrel_textures_);
    DestroyTextureSet(acorn_textures_);

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
