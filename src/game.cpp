#include "game.hpp"
#include "platform.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <random>

namespace fs = std::filesystem;

static const int kWindowWidth = 960;
static const int kWindowHeight = 540;
static const int kLevelWidth = 5200;


static const float kSquirrelShootMin = 1.8f;
static const float kSquirrelShootMax = 3.1f;
static const float kSquirrelShootAnimFrameDuration = 0.06f;
static const float kAcornSpeed = 420.0f;
static const float kAcornGravity = 240.0f;
static const float kAcornAnimFrameDuration = 0.08f;
static const float kPlayerHitCooldown = 0.9f;
static const float kSquirrelAggroRange = 520.0f;

struct TreeVisual {
    int x = 0;
    int trunk_w = 0;
    int trunk_h = 0;
    int canopy_w = 0;
    int canopy_h = 0;
};

static SDL_Texture* LoadTextureBMP(SDL_Renderer* renderer, const fs::path& path, int* out_w, int* out_h, bool use_black_colorkey) {
    if (!fs::exists(path)) {
        std::cerr << "File not found: " << path << "\n";
        return nullptr;
    }

    SDL_Surface* bmp = SDL_LoadBMP(path.string().c_str());
    if (!bmp) {
        std::cerr << "Failed to load " << path << ": " << SDL_GetError() << "\n";
        return nullptr;
    }

    if (use_black_colorkey) {
        SDL_SetColorKey(bmp, SDL_TRUE, SDL_MapRGB(bmp->format, 0, 0, 0));
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, bmp);
    if (texture && out_w && out_h) {
        *out_w = bmp->w;
        *out_h = bmp->h;
    }

    SDL_FreeSurface(bmp);
    return texture;
}

static TextureSet LoadSingleTexture(SDL_Renderer* renderer, const fs::path& path, bool use_black_colorkey) {
    TextureSet texture_set;
    SDL_Texture* texture = LoadTextureBMP(renderer, path, &texture_set.width, &texture_set.height, use_black_colorkey);
    if (texture) {
        texture_set.frames.push_back(texture);
    }
    return texture_set;
}

static TextureSet LoadTextureSet(SDL_Renderer* renderer, const std::vector<fs::path>& frame_paths, bool use_black_colorkey) {
    TextureSet texture_set;
    for (const fs::path& frame_path : frame_paths) {
        int frame_w = 0;
        int frame_h = 0;
        SDL_Texture* frame = LoadTextureBMP(renderer, frame_path, &frame_w, &frame_h, use_black_colorkey);
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

static void DrawHeart(SDL_Renderer* renderer, int x, int y, int scale, bool filled) {
    static const int pattern[7][8] = {
        {0,1,1,0,0,1,1,0},
        {1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1},
        {0,1,1,1,1,1,1,0},
        {0,0,1,1,1,1,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,0,0,0,0,0,0}
    };

    if (filled) {
        SDL_SetRenderDrawColor(renderer, 220, 40, 60, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
    }

    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (pattern[row][col] == 1) {
                SDL_Rect pixel{
                    x + col * scale,
                    y + row * scale,
                    scale,
                    scale
                };
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
}

static void DrawFilledCircle(SDL_Renderer* renderer, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; ++dy) {
        int dx_limit = static_cast<int>(std::sqrt(radius * radius - dy * dy));
        SDL_RenderDrawLine(renderer, cx - dx_limit, cy + dy, cx + dx_limit, cy + dy);
    }
}

static bool RectsIntersect(const SDL_Rect& a, const SDL_Rect& b) {
    return SDL_HasIntersection(&a, &b) == SDL_TRUE;
}

static float RandomRange(std::mt19937& rng, float low, float high) {
    std::uniform_real_distribution<float> dist(low, high);
    return dist(rng);
}

static void GenerateForestPlatforms(std::vector<Platform>& platforms) {
    platforms.clear();

    const int ground_y = kWindowHeight - 40;
    platforms.push_back({ SDL_Rect{0, ground_y, kLevelWidth, 40} });

    platforms.push_back({ SDL_Rect{200, 440, 140, 18} });
    platforms.push_back({ SDL_Rect{340, 390, 140, 18} });
    platforms.push_back({ SDL_Rect{500, 340, 140, 18} });
    platforms.push_back({ SDL_Rect{660, 290, 140, 18} });

    std::mt19937 rng(4000);

    int tree_x = 950;
    std::uniform_int_distribution<int> trunk_gap(240, 310);
    std::uniform_int_distribution<int> low_y(330, 390);
    std::uniform_int_distribution<int> mid_y(250, 315);
    std::uniform_int_distribution<int> high_y(180, 245);
    std::uniform_int_distribution<int> width_dist(115, 155);
    std::uniform_int_distribution<int> offset_dist(-22, 22);
    std::uniform_int_distribution<int> pattern_dist(0, 2);

    while (tree_x < kLevelWidth - 220) {
        int pattern = pattern_dist(rng);

        int branch1_y = low_y(rng);
        int branch2_y = mid_y(rng);
        int branch3_y = high_y(rng);

        int w1 = width_dist(rng);
        int w2 = width_dist(rng);
        int w3 = width_dist(rng);

        if (pattern == 0) {
            platforms.push_back({ SDL_Rect{tree_x - 55 + offset_dist(rng), branch1_y, w1, 18} });
            platforms.push_back({ SDL_Rect{tree_x + 30 + offset_dist(rng), branch2_y, w2, 18} });
            platforms.push_back({ SDL_Rect{tree_x - 20 + offset_dist(rng), branch3_y, w3, 18} });
        } else if (pattern == 1) {
            platforms.push_back({ SDL_Rect{tree_x + 20 + offset_dist(rng), branch1_y, w1, 18} });
            platforms.push_back({ SDL_Rect{tree_x - 60 + offset_dist(rng), branch2_y, w2, 18} });
            platforms.push_back({ SDL_Rect{tree_x + 40 + offset_dist(rng), branch3_y, w3, 18} });
        } else {
            platforms.push_back({ SDL_Rect{tree_x - 45 + offset_dist(rng), branch1_y, w1, 18} });
            platforms.push_back({ SDL_Rect{tree_x + 10 + offset_dist(rng), branch2_y, w2, 18} });
            platforms.push_back({ SDL_Rect{tree_x + 55 + offset_dist(rng), branch3_y, w3, 18} });
        }

        tree_x += trunk_gap(rng);
    }

    platforms.push_back({ SDL_Rect{4650, 250, 130, 18} });
    platforms.push_back({ SDL_Rect{4850, 210, 120, 18} });
    platforms.push_back({ SDL_Rect{5030, 280, 135, 18} });
}

static std::vector<TreeVisual> BuildTreeVisualsFromPlatforms(const std::vector<Platform>& platforms) {
    std::vector<TreeVisual> trees;
    const int ground_y = kWindowHeight - 40;

    for (const auto& platform : platforms) {
        if (platform.rect.h >= 30) {
            continue;
        }

        int trunk_center_x = platform.rect.x + platform.rect.w / 2;
        bool merged = false;

        for (auto& tree : trees) {
            if (std::abs(tree.x - trunk_center_x) < 95) {
                int branch_top = platform.rect.y;
                int desired_h = ground_y - branch_top + 65;
                if (desired_h > tree.trunk_h) {
                    tree.trunk_h = desired_h;
                    tree.canopy_h = desired_h / 2;
                }
                merged = true;
                break;
            }
        }

        if (!merged) {
            TreeVisual tree;
            tree.x = trunk_center_x;
            tree.trunk_w = 26;
            tree.trunk_h = ground_y - platform.rect.y + 70;
            tree.canopy_w = 130;
            tree.canopy_h = 90;
            trees.push_back(tree);
        }
    }

    return trees;
}

static void PopulateVines(const std::vector<Platform>& platforms, std::vector<Vine>& vines) {
    vines.clear();
    std::mt19937 rng(6060);

    std::uniform_int_distribution<int> chance_dist(0, 99);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> extra_len_dist(0, 70);

    for (const Platform& p : platforms) {
        if (p.rect.h >= 30) continue;    
        if (p.rect.x < 700) continue;     
        if (p.rect.w < 100) continue;
        if (chance_dist(rng) >= 30) continue;

        Vine v;

        int offset = (side_dist(rng) == 0) ? -18 : 18;

        int vine_x = p.rect.x + p.rect.w / 2 + offset;
        int vine_y = p.rect.y + 6;

        int vine_h = 110 + extra_len_dist(rng);

        if (p.rect.y < 260) {
            vine_h += 40;
        }

        int max_h = (kWindowHeight - 40) - vine_y;
        if (vine_h > max_h) {
            vine_h = max_h;
        }

        v.rect = SDL_Rect{ vine_x, vine_y, 6, vine_h };
        vines.push_back(v);

        if (vines.size() >= 18) {
            break;
        }
    }
}

static void PopulateSquirrels(const std::vector<Platform>& platforms, std::vector<SquirrelEnemy>& squirrels) {
    squirrels.clear();
    std::mt19937 rng(2026);

    for (const Platform& p : platforms) {
        if (p.rect.h >= 30) continue;
        if (p.rect.y > 340) continue;
        if (p.rect.x < 1100) continue;
        if (p.rect.x > 4700) continue;

        std::uniform_int_distribution<int> chance_dist(0, 99);
        if (chance_dist(rng) >= 18) continue;

        SquirrelEnemy s;
        s.x = static_cast<float>(p.rect.x + p.rect.w / 2);
        s.y = static_cast<float>(p.rect.y);
        s.shoot_timer = RandomRange(rng, kSquirrelShootMin, kSquirrelShootMax);
        s.shoot_anim_timer = 0.0f;
        s.shoot_frame = 0;
        s.facing_left = true;
        s.shooting = false;
        squirrels.push_back(s);

        if (squirrels.size() >= 6) {
            break;
        }
    }
}

static void PopulateSpikes(const std::vector<Platform>& platforms, std::vector<SpikeTrap>& spikes) {
    spikes.clear();
    std::mt19937 rng(7070);

    for (const Platform& p : platforms) {
        if (p.rect.h >= 30) continue;
        if (p.rect.x < 900) continue;
        if (p.rect.w < 90) continue;

        std::uniform_int_distribution<int> chance_dist(0, 99);
        if (chance_dist(rng) >= 18) continue;

        SpikeTrap spike;
        int spike_w = 34;
        int spike_h = 26;

        spike.rect = SDL_Rect{
            p.rect.x + (p.rect.w / 2) - (spike_w / 2),
            p.rect.y - spike_h,
            spike_w,
            spike_h
        };

        spikes.push_back(spike);
        if (spikes.size() >= 10) {
            break;
        }
    }
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

    player_texture_ = LoadSingleTexture(renderer_, assets_dir / "Opanda.bmp", false);

    std::vector<fs::path> idle_frames = CollectFramesByPrefix(assets_dir / "idel", "idel");
    if (idle_frames.empty()) idle_frames = CollectFramesByPrefix(assets_dir, "idel");
    idle_textures_ = LoadTextureSet(renderer_, idle_frames, false);

    std::vector<fs::path> walk_frames = CollectFramesByPrefix(assets_dir / "walk", "walk");
    if (walk_frames.empty()) walk_frames = CollectFramesByPrefix(assets_dir / "walk", "rewalk");
    if (walk_frames.empty()) walk_frames = CollectFramesByPrefix(assets_dir, "rewalk");
    walk_textures_ = LoadTextureSet(renderer_, walk_frames, false);

    std::vector<fs::path> jump_frames = CollectFramesByPrefix(assets_dir / "jump", "jump");
    if (jump_frames.empty()) jump_frames = CollectFramesByPrefix(assets_dir, "jump");
    jump_textures_ = LoadTextureSet(renderer_, jump_frames, false);

    std::vector<fs::path> punch_frames = CollectFramesByPrefix(assets_dir / "punch", "punch");
    if (punch_frames.empty()) punch_frames = CollectFramesByPrefix(assets_dir, "punch");
    punch_textures_ = LoadTextureSet(renderer_, punch_frames, false);

    std::vector<fs::path> heel_kick_frames = CollectFramesByPrefix(assets_dir / "heel", "heel");
    if (heel_kick_frames.empty()) heel_kick_frames = CollectFramesByPrefix(assets_dir, "heel");
    heel_kick_textures_ = LoadTextureSet(renderer_, heel_kick_frames, false);

    std::vector<fs::path> squirrel_frames = CollectFramesByPrefix(assets_dir / "squirrelshot", "shoot");
    squirrel_textures_ = LoadTextureSet(renderer_, squirrel_frames, true);

    std::vector<fs::path> acorn_frames = CollectFramesByPrefix(assets_dir / "acorn", "acorn");
    acorn_textures_ = LoadTextureSet(renderer_, acorn_frames, true);

    spike_texture_ = LoadSingleTexture(renderer_, assets_dir / "spikes" / "spike1.bmp", true);

    if (!player_texture_.Empty()) player_.SetTexture(player_texture_);
    if (!idle_textures_.Empty()) player_.SetIdleTextures(idle_textures_);
    if (!walk_textures_.Empty()) player_.SetWalkTextures(walk_textures_);
    if (!jump_textures_.Empty()) player_.SetJumpTextures(jump_textures_);
    if (!punch_textures_.Empty()) player_.SetPunchTextures(punch_textures_);
    if (!heel_kick_textures_.Empty()) player_.SetHeelKickTextures(heel_kick_textures_);

    player_.SetGroundY(kWindowHeight - 40.0f);
    player_.SetPosition(120.0f, kWindowHeight - 80.0f);

    GenerateForestPlatforms(platforms_);
    PopulateSquirrels(platforms_, squirrels_);
    PopulateSpikes(platforms_, spikes_);
    PopulateVines(platforms_, vines_);

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

void Game::HandleEvents() {
    SDL_Event e;
    input_.ClearFrame();

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running_ = false;
        } else if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            input_.OnKeyDown(e.key.keysym.sym);
            if (e.key.keysym.sym == SDLK_h) {
                player_.TakeDamage(1);
            }
        } else if (e.type == SDL_KEYUP) {
            input_.OnKeyUp(e.key.keysym.sym);
        }
    }
}

void Game::Update(float dt) {
    static std::mt19937 squirrel_rng(4242);

    player_.Update(dt, input_);
    player_.CheckPlatformCollisions(platforms_);

    if (player_damage_cooldown_ > 0.0f) {
        player_damage_cooldown_ -= dt;
        if (player_damage_cooldown_ < 0.0f) {
            player_damage_cooldown_ = 0.0f;
        }
    }

    for (auto& squirrel : squirrels_) {
        float dx = player_.GetX() - squirrel.x;

        if (std::fabs(dx) > 18.0f) {
            squirrel.facing_left = (dx > 0.0f);
        }

        bool player_in_range = std::fabs(dx) < kSquirrelAggroRange;

        if (!squirrel.shooting) {
            squirrel.shoot_timer -= dt;

            if (player_in_range && squirrel.shoot_timer <= 0.0f) {
                squirrel.shooting = true;
                squirrel.shoot_frame = 0;
                squirrel.shoot_anim_timer = 0.0f;
            } else if (!player_in_range && squirrel.shoot_timer <= 0.0f) {
                squirrel.shoot_timer = RandomRange(squirrel_rng, 0.6f, 1.4f);
            }
        }

        if (squirrel.shooting && !squirrel_textures_.Empty()) {
            squirrel.shoot_anim_timer += dt;

            if (squirrel.shoot_anim_timer >= kSquirrelShootAnimFrameDuration) {
                squirrel.shoot_anim_timer = 0.0f;
                ++squirrel.shoot_frame;

                if (squirrel.shoot_frame >= static_cast<int>(squirrel_textures_.frames.size())) {
                    squirrel.shooting = false;
                    squirrel.shoot_frame = 0;
                    squirrel.shoot_timer = RandomRange(squirrel_rng, kSquirrelShootMin, kSquirrelShootMax);

                    AcornProjectile acorn;

                    acorn.x = squirrel.x + (squirrel.facing_left ? -16.0f : 16.0f);
                    acorn.y = squirrel.y - 34.0f;

                    float target_x = player_.GetX() + player_.GetWidth() * 0.5f;
                    float target_y = player_.GetY() - player_.GetHeight() * 0.5f;

                    float aim_dx = target_x - acorn.x;
                    float aim_dy = target_y - acorn.y;

                    float len = std::sqrt(aim_dx * aim_dx + aim_dy * aim_dy);
                    if (len < 1.0f) {
                        len = 1.0f;
                    }

                    aim_dx /= len;
                    aim_dy /= len;

                    acorn.vx = aim_dx * kAcornSpeed;
                    acorn.vy = aim_dy * kAcornSpeed - 40.0f;

                    acorn.anim_timer = 0.0f;
                    acorn.anim_frame = 0;
                    acorn.active = true;
                    acorns_.push_back(acorn);
                }
            }
        }
    }

    SDL_Rect player_rect{
        static_cast<int>(player_.GetX()) + 8,
        static_cast<int>(player_.GetY()) - player_.GetHeight() + 6,
        player_.GetWidth() - 16,
        player_.GetHeight() - 8
    };

    bool on_vine = false;

    for (const auto& vine : vines_) {
        SDL_Rect vine_hitbox = vine.rect;
        vine_hitbox.x -= 8;
        vine_hitbox.w += 16;

        if (RectsIntersect(player_rect, vine_hitbox)) {
            on_vine = true;
            break;
        }
    }

    if (on_vine) {
        if (input_.move_up) {
            player_.SetVelocityY(-180.0f);
        } else if (input_.move_down) {
            player_.SetVelocityY(160.0f);
        } else {
            player_.SetVelocityY(20.0f);
        }
    }

    for (const auto& spike : spikes_) {
        SDL_Rect spike_hitbox = spike.rect;
        spike_hitbox.x += 10;
        spike_hitbox.w -= 20;
        spike_hitbox.y += 10;
        spike_hitbox.h -= 14;

        if (RectsIntersect(player_rect, spike_hitbox)) {
            player_.TakeDamage(player_.GetMaxHealth());
            break;
        }
    }

    for (auto& acorn : acorns_) {
        if (!acorn.active) continue;

        acorn.vy += kAcornGravity * dt;
        acorn.x += acorn.vx * dt;
        acorn.y += acorn.vy * dt;

        if (!acorn_textures_.Empty()) {
            acorn.anim_timer += dt;
            if (acorn.anim_timer >= kAcornAnimFrameDuration) {
                acorn.anim_timer = 0.0f;
                acorn.anim_frame = (acorn.anim_frame + 1) % static_cast<int>(acorn_textures_.frames.size());
            }
        }

        SDL_Rect acorn_rect{
            static_cast<int>(acorn.x) - 10,
            static_cast<int>(acorn.y) - 10,
            20,
            20
        };

        if (RectsIntersect(acorn_rect, player_rect)) {
            acorn.active = false;
            if (player_damage_cooldown_ <= 0.0f) {
                player_.TakeDamage(1);
                player_damage_cooldown_ = kPlayerHitCooldown;
            }
            continue;
        }

        if (acorn.x < -50.0f || acorn.x > static_cast<float>(kLevelWidth + 50) ||
            acorn.y > static_cast<float>(kWindowHeight + 120)) {
            acorn.active = false;
            continue;
        }

        if (acorn.y >= (kWindowHeight - 40)) {
            acorn.active = false;
        }
    }

    acorns_.erase(
        std::remove_if(acorns_.begin(), acorns_.end(),
            [](const AcornProjectile& a) { return !a.active; }),
        acorns_.end()
    );

    camera_x_ = player_.GetX() - (kWindowWidth / 2.0f);
    if (camera_x_ < 0.0f) {
        camera_x_ = 0.0f;
    }

    float max_camera_x = static_cast<float>(kLevelWidth - kWindowWidth);
    if (camera_x_ > max_camera_x) {
        camera_x_ = max_camera_x;
    }
}

void Game::Render() {
    SDL_SetRenderDrawColor(renderer_, 120, 185, 120, 255);
    SDL_RenderClear(renderer_);

    SDL_SetRenderDrawColor(renderer_, 140, 205, 150, 255);
    SDL_Rect skyBand1{0, 0, kWindowWidth, 170};
    SDL_RenderFillRect(renderer_, &skyBand1);

    SDL_SetRenderDrawColor(renderer_, 105, 170, 120, 255);
    SDL_Rect skyBand2{0, 170, kWindowWidth, 130};
    SDL_RenderFillRect(renderer_, &skyBand2);

    std::vector<TreeVisual> trees = BuildTreeVisualsFromPlatforms(platforms_);

    for (const auto& tree : trees) {
        int screen_x = tree.x - static_cast<int>(camera_x_ * 0.55f);

        SDL_SetRenderDrawColor(renderer_, 55, 85, 55, 255);
        SDL_Rect trunk{
            screen_x - 16,
            kWindowHeight - 40 - tree.trunk_h + 40,
            20,
            tree.trunk_h - 40
        };
        SDL_RenderFillRect(renderer_, &trunk);

        SDL_SetRenderDrawColor(renderer_, 62, 104, 60, 255);
        DrawFilledCircle(renderer_, screen_x, trunk.y + 22, 44);
        DrawFilledCircle(renderer_, screen_x - 34, trunk.y + 46, 30);
        DrawFilledCircle(renderer_, screen_x + 34, trunk.y + 46, 30);

        SDL_SetRenderDrawColor(renderer_, 78, 126, 74, 255);
        DrawFilledCircle(renderer_, screen_x - 10, trunk.y + 28, 22);
        DrawFilledCircle(renderer_, screen_x + 16, trunk.y + 30, 18);
    }

    SDL_SetRenderDrawColor(renderer_, 58, 88, 42, 255);
    SDL_Rect dirtBand{0, kWindowHeight - 52, kWindowWidth, 12};
    SDL_RenderFillRect(renderer_, &dirtBand);

    SDL_SetRenderDrawColor(renderer_, 75, 110, 58, 255);
    SDL_Rect ground{0, kWindowHeight - 40, kWindowWidth, 40};
    SDL_RenderFillRect(renderer_, &ground);

    for (const auto& tree : trees) {
        int screen_x = tree.x - static_cast<int>(camera_x_);

        SDL_Rect trunk{
            screen_x - tree.trunk_w / 2,
            kWindowHeight - 40 - tree.trunk_h,
            tree.trunk_w,
            tree.trunk_h
        };

        SDL_SetRenderDrawColor(renderer_, 92, 58, 34, 255);
        SDL_RenderFillRect(renderer_, &trunk);

        SDL_SetRenderDrawColor(renderer_, 120, 78, 46, 255);
        SDL_Rect trunkHighlight{
            trunk.x + trunk.w / 3,
            trunk.y,
            trunk.w / 4,
            trunk.h
        };
        SDL_RenderFillRect(renderer_, &trunkHighlight);

        SDL_SetRenderDrawColor(renderer_, 70, 42, 24, 255);
        for (int i = 4; i < trunk.h; i += 22) {
            SDL_RenderDrawLine(renderer_, trunk.x + 4, trunk.y + i, trunk.x + trunk.w - 4, trunk.y + i + 3);
        }

        SDL_SetRenderDrawColor(renderer_, 80, 50, 28, 255);
        SDL_RenderDrawLine(renderer_, trunk.x + 4, trunk.y + trunk.h, trunk.x - 10, trunk.y + trunk.h + 10);
        SDL_RenderDrawLine(renderer_, trunk.x + trunk.w - 4, trunk.y + trunk.h, trunk.x + trunk.w + 10, trunk.y + trunk.h + 10);
        SDL_RenderDrawLine(renderer_, trunk.x + trunk.w / 2, trunk.y + trunk.h, trunk.x + trunk.w / 2 - 8, trunk.y + trunk.h + 12);

        SDL_SetRenderDrawColor(renderer_, 88, 54, 30, 255);
        SDL_RenderDrawLine(renderer_, trunk.x, trunk.y + trunk.h / 3, trunk.x - 16, trunk.y + trunk.h / 3 - 8);
        SDL_RenderDrawLine(renderer_, trunk.x + trunk.w, trunk.y + trunk.h / 2, trunk.x + trunk.w + 16, trunk.y + trunk.h / 2 - 6);

        SDL_SetRenderDrawColor(renderer_, 34, 102, 40, 255);
        DrawFilledCircle(renderer_, screen_x, trunk.y + 4, 50);
        DrawFilledCircle(renderer_, screen_x - 42, trunk.y + 34, 38);
        DrawFilledCircle(renderer_, screen_x + 42, trunk.y + 30, 38);
        DrawFilledCircle(renderer_, screen_x - 18, trunk.y + 58, 34);
        DrawFilledCircle(renderer_, screen_x + 18, trunk.y + 56, 34);

        SDL_SetRenderDrawColor(renderer_, 48, 128, 52, 255);
        DrawFilledCircle(renderer_, screen_x, trunk.y + 12, 42);
        DrawFilledCircle(renderer_, screen_x - 34, trunk.y + 32, 34);
        DrawFilledCircle(renderer_, screen_x + 34, trunk.y + 30, 34);
        DrawFilledCircle(renderer_, screen_x, trunk.y + 52, 32);

        SDL_SetRenderDrawColor(renderer_, 72, 162, 74, 255);
        DrawFilledCircle(renderer_, screen_x - 14, trunk.y + 16, 20);
        DrawFilledCircle(renderer_, screen_x + 22, trunk.y + 22, 18);
        DrawFilledCircle(renderer_, screen_x - 28, trunk.y + 42, 16);

        SDL_SetRenderDrawColor(renderer_, 26, 92, 34, 255);
        SDL_RenderDrawLine(renderer_, screen_x - 24, trunk.y + 48, screen_x - 28, trunk.y + 92);
        SDL_RenderDrawLine(renderer_, screen_x + 18, trunk.y + 44, screen_x + 22, trunk.y + 84);
    }


    for (const auto& vine : vines_) {
        SDL_Rect r = vine.rect;
        r.x -= static_cast<int>(camera_x_);

        SDL_SetRenderDrawColor(renderer_, 34, 120, 44, 255);
        SDL_RenderFillRect(renderer_, &r);

        SDL_SetRenderDrawColor(renderer_, 58, 160, 62, 255);
        SDL_Rect highlight{ r.x + 2, r.y, 2, r.h };
        SDL_RenderFillRect(renderer_, &highlight);

        SDL_SetRenderDrawColor(renderer_, 48, 145, 52, 255);
        for (int y = r.y + 10; y < r.y + r.h - 8; y += 18) {
            SDL_RenderDrawLine(renderer_, r.x + 2, y, r.x - 6, y + 4);
            SDL_RenderDrawLine(renderer_, r.x + 3, y + 2, r.x + 9, y + 6);
        }
    }

    for (const auto& platform : platforms_) {
        SDL_Rect screenRect = platform.rect;
        screenRect.x -= static_cast<int>(camera_x_);

        if (platform.rect.h >= 30) {
            continue;
        }

        SDL_SetRenderDrawColor(renderer_, 110, 72, 42, 255);
        SDL_RenderFillRect(renderer_, &screenRect);

        SDL_SetRenderDrawColor(renderer_, 138, 94, 58, 255);
        SDL_Rect topBark = screenRect;
        topBark.h = 5;
        SDL_RenderFillRect(renderer_, &topBark);

        SDL_SetRenderDrawColor(renderer_, 42, 120, 45, 255);
        SDL_Rect moss{
            screenRect.x + 8,
            screenRect.y - 6,
            screenRect.w - 16,
            6
        };
        SDL_RenderFillRect(renderer_, &moss);

        SDL_SetRenderDrawColor(renderer_, 58, 150, 58, 255);
        for (int gx = screenRect.x + 10; gx < screenRect.x + screenRect.w - 10; gx += 12) {
            SDL_RenderDrawLine(renderer_, gx, screenRect.y, gx + 2, screenRect.y - 5);
            SDL_RenderDrawLine(renderer_, gx + 4, screenRect.y, gx + 5, screenRect.y - 6);
        }
    }

    for (const auto& spike : spikes_) {
        SDL_Rect dest = spike.rect;
        dest.x -= static_cast<int>(camera_x_);

        SDL_Texture* tex = spike_texture_.First();
        if (tex) {
            SDL_RenderCopy(renderer_, tex, nullptr, &dest);
        } else {
            SDL_SetRenderDrawColor(renderer_, 190, 190, 190, 255);
            SDL_RenderFillRect(renderer_, &dest);
        }
    }

    for (const auto& squirrel : squirrels_) {
        int draw_w = squirrel_textures_.width > 0 ? squirrel_textures_.width : 54;
        int draw_h = squirrel_textures_.height > 0 ? squirrel_textures_.height : 54;

        SDL_Rect dest{
            static_cast<int>(squirrel.x - camera_x_) - draw_w / 2,
            static_cast<int>(squirrel.y) - draw_h,
            draw_w,
            draw_h
        };

        SDL_Texture* tex = nullptr;
        if (!squirrel_textures_.Empty()) {
            int frame = squirrel.shooting ? squirrel.shoot_frame : 0;
            frame = std::clamp(frame, 0, static_cast<int>(squirrel_textures_.frames.size()) - 1);
            tex = squirrel_textures_.frames[frame];
        }

        if (tex) {
            SDL_RendererFlip flip = squirrel.facing_left ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderCopyEx(renderer_, tex, nullptr, &dest, 0.0, nullptr, flip);
        } else {
            SDL_SetRenderDrawColor(renderer_, 120, 80, 50, 255);
            SDL_RenderFillRect(renderer_, &dest);
        }
    }

    for (const auto& acorn : acorns_) {
        int draw_w = acorn_textures_.width > 0 ? acorn_textures_.width : 20;
        int draw_h = acorn_textures_.height > 0 ? acorn_textures_.height : 20;

        SDL_Rect dest{
            static_cast<int>(acorn.x - camera_x_) - draw_w / 2,
            static_cast<int>(acorn.y) - draw_h / 2,
            draw_w,
            draw_h
        };

        SDL_Texture* tex = acorn_textures_.Empty()
            ? nullptr
            : acorn_textures_.frames[acorn.anim_frame % acorn_textures_.frames.size()];

        if (tex) {
            SDL_RenderCopy(renderer_, tex, nullptr, &dest);
        } else {
            SDL_SetRenderDrawColor(renderer_, 160, 100, 40, 255);
            SDL_RenderFillRect(renderer_, &dest);
        }
    }

    player_.Render(renderer_, camera_x_);

    int health = player_.GetHealth();
    int max_health = player_.GetMaxHealth();
    for (int i = 0; i < max_health; ++i) {
        DrawHeart(renderer_, 20 + i * 36, 20, 4, i < health);
    }

    SDL_RenderPresent(renderer_);
}

void Game::Shutdown() {
    DestroyTextureSet(idle_textures_);
    DestroyTextureSet(walk_textures_);
    DestroyTextureSet(jump_textures_);
    DestroyTextureSet(punch_textures_);
    DestroyTextureSet(heel_kick_textures_);
    DestroyTextureSet(player_texture_);
    DestroyTextureSet(squirrel_textures_);
    DestroyTextureSet(acorn_textures_);
    DestroyTextureSet(spike_texture_);

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