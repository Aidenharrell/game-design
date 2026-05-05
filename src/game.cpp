#include "game.hpp"
#include "platform.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {
constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 540;
constexpr int kLevelWidth = 5200;
constexpr float kPlayerHitCooldown = 0.9f;

struct TreeVisual {
    int x = 0;
    int trunk_w = 0;
    int trunk_h = 0;
    int canopy_w = 0;
    int canopy_h = 0;
};

struct LevelPhase {
    int start_x = 0;
    int end_x = 0;
    int ground_r = 0;
    int ground_g = 0;
    int ground_b = 0;
};

constexpr LevelPhase kLevelPhases[] = {
    {0, 1400, 75, 110, 58},
    {1400, 2600, 86, 104, 56},
    {2600, 3900, 99, 94, 54},
    {3900, kLevelWidth, 112, 84, 54}
};
constexpr int kLevelPhaseCount = sizeof(kLevelPhases) / sizeof(kLevelPhases[0]);

int DifficultyPhaseForX(int x) {
    for (int i = kLevelPhaseCount - 1; i >= 0; --i) {
        if (x >= kLevelPhases[i].start_x) {
            return i;
        }
    }
    return 0;
}

SDL_Texture* LoadTextureBMP(SDL_Renderer* renderer, const fs::path& path, int* out_w, int* out_h, bool use_black_colorkey) {
    if (!fs::exists(path)) {
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

TextureSet LoadSingleTexture(SDL_Renderer* renderer, const fs::path& path, bool use_black_colorkey) {
    TextureSet texture_set;
    SDL_Texture* texture = LoadTextureBMP(renderer, path, &texture_set.width, &texture_set.height, use_black_colorkey);
    if (texture) {
        texture_set.frames.push_back(texture);
    }
    return texture_set;
}

TextureSet LoadSingleTexture(SDL_Renderer* renderer, const std::vector<fs::path>& candidate_paths, bool use_black_colorkey) {
    for (const fs::path& candidate : candidate_paths) {
        TextureSet texture_set = LoadSingleTexture(renderer, candidate, use_black_colorkey);
        if (!texture_set.Empty()) {
            return texture_set;
        }
    }
    return {};
}

TextureSet LoadTextureSet(SDL_Renderer* renderer, const std::vector<fs::path>& frame_paths, bool use_black_colorkey) {
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

void DestroyTextureSet(TextureSet& texture_set) {
    for (SDL_Texture* texture : texture_set.frames) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
    texture_set.frames.clear();
    texture_set.width = 0;
    texture_set.height = 0;
}

fs::path ResolveAssetsDir() {
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

std::vector<fs::path> CollectFramesByPrefix(const fs::path& dir, const std::string& prefix) {
    std::vector<fs::path> frames;
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return frames;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

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

    auto frame_index = [&](const fs::path& path) -> int {
        const std::string stem = path.stem().string();
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
        if (ai != bi) {
            return ai < bi;
        }
        return a.filename().string() < b.filename().string();
    });

    return frames;
}

std::vector<fs::path> CollectFramesByPrefix(const std::vector<fs::path>& dirs, const std::string& prefix) {
    for (const fs::path& dir : dirs) {
        std::vector<fs::path> frames = CollectFramesByPrefix(dir, prefix);
        if (!frames.empty()) {
            return frames;
        }
    }
    return {};
}

void DrawHeart(SDL_Renderer* renderer, int x, int y, int scale, bool filled) {
    static const int pattern[7][8] = {
        {0, 1, 1, 0, 0, 1, 1, 0},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {0, 1, 1, 1, 1, 1, 1, 0},
        {0, 0, 1, 1, 1, 1, 0, 0},
        {0, 0, 0, 1, 1, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0}
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

void DrawFilledCircle(SDL_Renderer* renderer, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; ++dy) {
        int dx_limit = static_cast<int>(std::sqrt(radius * radius - dy * dy));
        SDL_RenderDrawLine(renderer, cx - dx_limit, cy + dy, cx + dx_limit, cy + dy);
    }
}

bool RectsIntersect(const SDL_Rect& a, const SDL_Rect& b) {
    return SDL_HasIntersection(&a, &b) == SDL_TRUE;
}

void GenerateForestPlatforms(std::vector<Platform>& platforms) {
    platforms.clear();

    const int ground_y = kWindowHeight - 40;
    platforms.push_back({ SDL_Rect{0, ground_y, kLevelWidth, 40} });

    platforms.push_back({ SDL_Rect{200, 440, 140, 18} });
    platforms.push_back({ SDL_Rect{340, 390, 140, 18} });
    platforms.push_back({ SDL_Rect{500, 340, 140, 18} });
    platforms.push_back({ SDL_Rect{660, 290, 140, 18} });

    std::mt19937 rng(4000);
    int tree_x = 950;
    std::uniform_int_distribution<int> offset_dist(-22, 22);
    std::uniform_int_distribution<int> pattern_dist(0, 2);

    while (tree_x < kLevelWidth - 220) {
        const int phase = DifficultyPhaseForX(tree_x);
        const int gap_min[] = {240, 260, 285, 315};
        const int gap_max[] = {315, 345, 380, 420};
        const int low_min[] = {330, 315, 300, 285};
        const int low_max[] = {390, 375, 360, 340};
        const int mid_min[] = {250, 235, 215, 195};
        const int mid_max[] = {315, 295, 275, 250};
        const int high_min[] = {180, 165, 145, 125};
        const int high_max[] = {245, 225, 205, 185};
        const int width_min[] = {120, 110, 98, 86};
        const int width_max[] = {160, 145, 130, 115};

        std::uniform_int_distribution<int> trunk_gap(gap_min[phase], gap_max[phase]);
        std::uniform_int_distribution<int> low_y(low_min[phase], low_max[phase]);
        std::uniform_int_distribution<int> mid_y(mid_min[phase], mid_max[phase]);
        std::uniform_int_distribution<int> high_y(high_min[phase], high_max[phase]);
        std::uniform_int_distribution<int> width_dist(width_min[phase], width_max[phase]);

        const int pattern = pattern_dist(rng);
        const int branch1_y = low_y(rng);
        const int branch2_y = mid_y(rng);
        const int branch3_y = high_y(rng);
        const int w1 = width_dist(rng);
        const int w2 = width_dist(rng);
        const int w3 = width_dist(rng);

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

std::vector<TreeVisual> BuildTreeVisualsFromPlatforms(const std::vector<Platform>& platforms) {
    std::vector<TreeVisual> trees;
    const int ground_y = kWindowHeight - 40;

    for (const auto& platform : platforms) {
        if (platform.rect.h >= 30) {
            continue;
        }

        const int trunk_center_x = platform.rect.x + platform.rect.w / 2;
        bool merged = false;

        for (auto& tree : trees) {
            if (std::abs(tree.x - trunk_center_x) < 95) {
                const int branch_top = platform.rect.y;
                const int desired_h = ground_y - branch_top + 65;
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

void PopulateVines(const std::vector<Platform>& platforms, std::vector<Vine>& vines) {
    vines.clear();
    std::mt19937 rng(6060);
    std::uniform_int_distribution<int> chance_dist(0, 99);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> extra_len_dist(0, 70);

    for (const Platform& platform : platforms) {
        if (platform.rect.h >= 30 || platform.rect.x < 700 || platform.rect.w < 100) {
            continue;
        }
        if (chance_dist(rng) >= 30) {
            continue;
        }

        Vine vine;
        const int offset = side_dist(rng) == 0 ? -18 : 18;
        const int vine_x = platform.rect.x + platform.rect.w / 2 + offset;
        const int vine_y = platform.rect.y + 6;
        int vine_h = 110 + extra_len_dist(rng);
        if (platform.rect.y < 260) {
            vine_h += 40;
        }

        const int max_h = (kWindowHeight - 40) - vine_y;
        vine_h = std::min(vine_h, max_h);
        vine.rect = SDL_Rect{ vine_x, vine_y, 6, vine_h };
        vines.push_back(vine);

        if (vines.size() >= 18) {
            break;
        }
    }
}

void PopulateSpikes(const std::vector<Platform>& platforms, std::vector<SpikeTrap>& spikes) {
    spikes.clear();
    std::mt19937 rng(7070);
    std::uniform_int_distribution<int> chance_dist(0, 99);
    constexpr int spike_w = 34;
    constexpr int spike_h = 26;
    constexpr int ground_y = kWindowHeight - 40;

    struct BottomSpikePhase {
        int start_x = 0;
        int end_x = 0;
        int spacing = 0;
        int cluster_size = 0;
        int jitter = 0;
    };

    const BottomSpikePhase bottom_phases[] = {
        {760, 1400, 360, 1, 42},
        {1500, 2600, 290, 2, 58},
        {2700, 3900, 230, 3, 70},
        {4000, kLevelWidth - 260, 185, 4, 64}
    };

    for (const BottomSpikePhase& phase : bottom_phases) {
        int cluster_index = 0;
        for (int x = phase.start_x; x < phase.end_x; x += phase.spacing) {
            const int offset = ((cluster_index * 37) % (phase.jitter + 1)) - (phase.jitter / 2);
            for (int i = 0; i < phase.cluster_size; ++i) {
                SpikeTrap spike;
                spike.rect = SDL_Rect{
                    x + offset + i * (spike_w + 6),
                    ground_y - spike_h,
                    spike_w,
                    spike_h
                };
                spikes.push_back(spike);
            }
            ++cluster_index;
        }
    }

    int elevated_spike_count = 0;
    for (const Platform& platform : platforms) {
        if (platform.rect.h >= 30 || platform.rect.x < 900 || platform.rect.w < 90) {
            continue;
        }

        const int phase = DifficultyPhaseForX(platform.rect.x);
        const int platform_spike_chance[] = {16, 24, 34, 46};
        if (chance_dist(rng) >= platform_spike_chance[phase]) {
            continue;
        }

        SpikeTrap spike;
        spike.rect = SDL_Rect{
            platform.rect.x + (platform.rect.w / 2) - (spike_w / 2),
            platform.rect.y - spike_h,
            spike_w,
            spike_h
        };
        spikes.push_back(spike);
        ++elevated_spike_count;

        if (elevated_spike_count >= 14) {
            break;
        }
    }
}

void PopulatePoisonGas(std::vector<PoisonGasCloud>& poison_gas) {
    poison_gas.clear();
    constexpr int ground_y = kWindowHeight - 40;

    struct GasPhase {
        int start_x = 0;
        int end_x = 0;
        int spacing = 0;
        int width = 0;
        int height = 0;
        int jitter = 0;
    };

    const GasPhase gas_phases[] = {
        {1500, 2600, 430, 92, 34, 52},
        {2700, 3900, 340, 128, 44, 70},
        {4000, kLevelWidth - 260, 270, 168, 56, 84}
    };

    for (const GasPhase& phase : gas_phases) {
        int cloud_index = 0;
        for (int x = phase.start_x; x < phase.end_x; x += phase.spacing) {
            const int offset = ((cloud_index * 43) % (phase.jitter + 1)) - (phase.jitter / 2);
            PoisonGasCloud cloud;
            cloud.rect = SDL_Rect{
                x + offset,
                ground_y - phase.height,
                phase.width,
                phase.height
            };
            poison_gas.push_back(cloud);
            ++cloud_index;
        }
    }
}

void PopulateDeadlyVines(const std::vector<Platform>& platforms, std::vector<DeadlyVine>& deadly_vines) {
    deadly_vines.clear();
    std::mt19937 rng(9090);
    std::uniform_int_distribution<int> chance_dist(0, 99);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> length_bonus_dist(0, 60);

    int deadly_vine_count = 0;
    for (const Platform& platform : platforms) {
        if (platform.rect.h >= 30 || platform.rect.x < 1450 || platform.rect.w < 95) {
            continue;
        }

        const int phase = DifficultyPhaseForX(platform.rect.x);
        const int deadly_vine_chance[] = {0, 18, 30, 44};
        if (chance_dist(rng) >= deadly_vine_chance[phase]) {
            continue;
        }

        const int vine_x = platform.rect.x + platform.rect.w / 2 + (side_dist(rng) == 0 ? -32 : 32);
        const int vine_y = platform.rect.y + 8;
        int vine_h = 95 + phase * 18 + length_bonus_dist(rng);
        const int max_h = (kWindowHeight - 40) - vine_y;
        vine_h = std::min(vine_h, max_h);
        if (vine_h < 45) {
            continue;
        }

        DeadlyVine vine;
        vine.rect = SDL_Rect{vine_x, vine_y, 8, vine_h};
        deadly_vines.push_back(vine);
        ++deadly_vine_count;

        if (deadly_vine_count >= 16) {
            break;
        }
    }
}

void PopulateSquirrels(const std::vector<Platform>& platforms,
                       std::vector<SquirrelEnemy>& squirrels,
                       const TextureSet& squirrel_textures,
                       const TextureSet& acorn_textures) {
    squirrels.clear();
    std::mt19937 rng(2026);
    std::uniform_int_distribution<int> chance_dist(0, 99);

    for (const Platform& platform : platforms) {
        if (platform.rect.h >= 30 || platform.rect.y > 340 || platform.rect.x < 1100 || platform.rect.x > 4700) {
            continue;
        }
        if (chance_dist(rng) >= 18) {
            continue;
        }

        SquirrelEnemy squirrel;
        squirrel.SetPosition(static_cast<float>(platform.rect.x + platform.rect.w / 2), static_cast<float>(platform.rect.y));
        squirrel.SetSquirrelTextures(squirrel_textures);
        squirrel.SetAcornTextures(acorn_textures);
        squirrels.push_back(squirrel);

        if (squirrels.size() >= 6) {
            break;
        }
    }
}

void DrawDecorativeApples(SDL_Renderer* renderer,
                          SDL_Texture* apple_texture,
                          int apple_w,
                          int apple_h,
                          const std::vector<TreeVisual>& trees,
                          float camera_x) {
    if (!apple_texture || apple_w <= 0 || apple_h <= 0) {
        return;
    }

    for (std::size_t i = 0; i < trees.size(); ++i) {
        if (i % 2 != 0) {
            continue;
        }

        const TreeVisual& tree = trees[i];
        const int screen_x = tree.x - static_cast<int>(camera_x);
        const int trunk_top = kWindowHeight - 40 - tree.trunk_h;
        const int apple_y = trunk_top + 34 + static_cast<int>((i % 3) * 6);
        const int offsets[3] = { -26, 6, 24 };

        for (int offset : offsets) {
            SDL_Rect dest{
                screen_x + offset,
                apple_y + (offset == 6 ? 10 : 0),
                apple_w,
                apple_h
            };
            SDL_RenderCopy(renderer, apple_texture, nullptr, &dest);
        }
    }
}

void DrawGroundPhaseBands(SDL_Renderer* renderer, float camera_x) {
    for (const LevelPhase& phase : kLevelPhases) {
        const int screen_x = phase.start_x - static_cast<int>(camera_x);
        const int screen_end_x = phase.end_x - static_cast<int>(camera_x);

        if (screen_end_x < 0 || screen_x > kWindowWidth) {
            continue;
        }

        SDL_SetRenderDrawColor(
            renderer,
            static_cast<Uint8>(phase.ground_r),
            static_cast<Uint8>(phase.ground_g),
            static_cast<Uint8>(phase.ground_b),
            255);
        SDL_Rect phase_ground{
            std::max(0, screen_x),
            kWindowHeight - 40,
            std::min(kWindowWidth, screen_end_x) - std::max(0, screen_x),
            40
        };
        SDL_RenderFillRect(renderer, &phase_ground);
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

    const fs::path assets_dir = ResolveAssetsDir();

    player_texture_ = LoadSingleTexture(renderer_, assets_dir / "Opanda.bmp", false);
    background_texture_ = LoadSingleTexture(
        renderer_,
        {
            assets_dir / "background" / "Background.bmp",
            assets_dir / "Background.bmp"
        },
        false);
    apple_texture_ = LoadSingleTexture(
        renderer_,
        {
            assets_dir / "tree" / "apple.bmp",
            assets_dir / "apple.bmp"
        },
        true);
    spike_texture_ = LoadSingleTexture(
        renderer_,
        {
            assets_dir / "spikes" / "spike1.bmp",
            assets_dir / "spike1.bmp"
        },
        true);

    idle_textures_ = LoadTextureSet(
        renderer_,
        CollectFramesByPrefix({ assets_dir / "idel", assets_dir }, "idel"),
        false);
    walk_textures_ = LoadTextureSet(
        renderer_,
        CollectFramesByPrefix({ assets_dir / "walk", assets_dir }, "rewalk"),
        false);
    jump_textures_ = LoadTextureSet(
        renderer_,
        CollectFramesByPrefix({ assets_dir / "jump", assets_dir }, "jump"),
        false);
    punch_textures_ = LoadTextureSet(
        renderer_,
        CollectFramesByPrefix({ assets_dir / "punch", assets_dir }, "punch"),
        false);
    heel_kick_textures_ = LoadTextureSet(
        renderer_,
        CollectFramesByPrefix({ assets_dir / "heel", assets_dir }, "heel"),
        false);
    squirrel_textures_ = LoadTextureSet(
        renderer_,
        CollectFramesByPrefix({ assets_dir / "squirrelshot", assets_dir }, "shoot"),
        true);
    acorn_textures_ = LoadTextureSet(
        renderer_,
        CollectFramesByPrefix({ assets_dir / "acorn", assets_dir }, "acorn"),
        true);

    if (!player_texture_.Empty()) player_.SetTexture(player_texture_);
    if (!idle_textures_.Empty()) player_.SetIdleTextures(idle_textures_);
    if (!walk_textures_.Empty()) player_.SetWalkTextures(walk_textures_);
    if (!jump_textures_.Empty()) player_.SetJumpTextures(jump_textures_);
    if (!punch_textures_.Empty()) player_.SetPunchTextures(punch_textures_);
    if (!heel_kick_textures_.Empty()) player_.SetHeelKickTextures(heel_kick_textures_);

    ResetRun();

    running_ = true;
    return true;
}

void Game::ResetRun() {
    input_ = InputState{};
    camera_x_ = 0.0f;
    player_damage_cooldown_ = 0.0f;

    player_.SetGroundY(kWindowHeight - 40.0f);
    player_.ResetForRun(120.0f, kWindowHeight - 80.0f);

    GenerateForestPlatforms(platforms_);
    PopulateVines(platforms_, vines_);
    PopulateSpikes(platforms_, spikes_);
    PopulatePoisonGas(poison_gas_);
    PopulateDeadlyVines(platforms_, deadly_vines_);
    PopulateSquirrels(platforms_, squirrels_, squirrel_textures_, acorn_textures_);
}

void Game::Run() {
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 last = now;

    while (running_) {
        last = now;
        now = SDL_GetPerformanceCounter();
        const double freq = static_cast<double>(SDL_GetPerformanceFrequency());
        const float dt = static_cast<float>((now - last) / freq);

        HandleEvents();
        Update(dt);
        Render();
    }
}

void Game::HandleEvents() {
    SDL_Event event;
    input_.ClearFrame();

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running_ = false;
        } else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
            input_.OnKeyDown(event.key.keysym.sym);
            if (event.key.keysym.sym == SDLK_h) {
                player_.TakeDamage(1);
            }
        } else if (event.type == SDL_KEYUP) {
            input_.OnKeyUp(event.key.keysym.sym);
        }
    }
}

void Game::Update(float dt) {
    player_.Update(dt, input_);
    player_.CheckPlatformCollisions(platforms_);

    if (player_damage_cooldown_ > 0.0f) {
        player_damage_cooldown_ = std::max(0.0f, player_damage_cooldown_ - dt);
    }

    SDL_Rect player_rect = player_.GetBodyRect();
    const SDL_Rect attack_rect = player_.GetAttackRect();

    bool on_vine = false;
    for (const Vine& vine : vines_) {
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

    auto damage_player_from_hazard = [&](int amount, float knockback_x, float knockback_y) {
        if (player_damage_cooldown_ > 0.0f) {
            return false;
        }

        player_.TakeDamage(amount);
        player_.ApplyKnockback(knockback_x, knockback_y);
        player_damage_cooldown_ = kPlayerHitCooldown;
        return true;
    };

    for (SquirrelEnemy& squirrel : squirrels_) {
        squirrel.Update(dt, player_rect);

        if (attack_rect.w > 0 && attack_rect.h > 0) {
            squirrel.TryTakeHit(attack_rect);
        }

        float knockback_x = 0.0f;
        if (player_damage_cooldown_ <= 0.0f &&
            squirrel.CheckProjectileHitPlayer(player_rect, &knockback_x)) {
            player_.TakeDamage(1);
            player_.ApplyKnockback(knockback_x, -220.0f);
            player_damage_cooldown_ = kPlayerHitCooldown;
        }
    }

    player_rect = player_.GetBodyRect();
    for (const PoisonGasCloud& gas : poison_gas_) {
        if (RectsIntersect(player_rect, gas.rect)) {
            damage_player_from_hazard(1, 0.0f, -130.0f);
            break;
        }
    }

    player_rect = player_.GetBodyRect();
    for (const DeadlyVine& vine : deadly_vines_) {
        SDL_Rect vine_hitbox = vine.rect;
        vine_hitbox.x -= 6;
        vine_hitbox.w += 12;

        if (RectsIntersect(player_rect, vine_hitbox)) {
            const float knockback_x = player_.GetX() < static_cast<float>(vine.rect.x) ? -150.0f : 150.0f;
            damage_player_from_hazard(1, knockback_x, -160.0f);
            break;
        }
    }

    player_rect = player_.GetBodyRect();
    for (const SpikeTrap& spike : spikes_) {
        SDL_Rect spike_hitbox = spike.rect;
        spike_hitbox.x += 10;
        spike_hitbox.w -= 20;
        spike_hitbox.y += 10;
        spike_hitbox.h -= 14;

        if (RectsIntersect(player_rect, spike_hitbox)) {
            const float knockback_x = player_.GetX() < static_cast<float>(spike.rect.x) ? -130.0f : 130.0f;
            damage_player_from_hazard(1, knockback_x, -190.0f);
            break;
        }
    }

    if (player_.GetHealth() <= 0) {
        ResetRun();
        return;
    }

    camera_x_ = player_.GetX() - (kWindowWidth / 2.0f);
    if (camera_x_ < 0.0f) {
        camera_x_ = 0.0f;
    }

    const float max_camera_x = static_cast<float>(kLevelWidth - kWindowWidth);
    if (camera_x_ > max_camera_x) {
        camera_x_ = max_camera_x;
    }
}

void Game::Render() {
    if (!background_texture_.Empty()) {
        SDL_RenderCopy(renderer_, background_texture_.First(), nullptr, nullptr);
    } else {
        SDL_SetRenderDrawColor(renderer_, 120, 185, 120, 255);
        SDL_RenderClear(renderer_);

        SDL_SetRenderDrawColor(renderer_, 140, 205, 150, 255);
        SDL_Rect sky_band_1{0, 0, kWindowWidth, 170};
        SDL_RenderFillRect(renderer_, &sky_band_1);

        SDL_SetRenderDrawColor(renderer_, 105, 170, 120, 255);
        SDL_Rect sky_band_2{0, 170, kWindowWidth, 130};
        SDL_RenderFillRect(renderer_, &sky_band_2);
    }

    std::vector<TreeVisual> trees = BuildTreeVisualsFromPlatforms(platforms_);

    for (const TreeVisual& tree : trees) {
        const int screen_x = tree.x - static_cast<int>(camera_x_ * 0.55f);

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
    SDL_Rect dirt_band{0, kWindowHeight - 52, kWindowWidth, 12};
    SDL_RenderFillRect(renderer_, &dirt_band);

    DrawGroundPhaseBands(renderer_, camera_x_);

    for (const TreeVisual& tree : trees) {
        const int screen_x = tree.x - static_cast<int>(camera_x_);
        SDL_Rect trunk{
            screen_x - tree.trunk_w / 2,
            kWindowHeight - 40 - tree.trunk_h,
            tree.trunk_w,
            tree.trunk_h
        };

        SDL_SetRenderDrawColor(renderer_, 92, 58, 34, 255);
        SDL_RenderFillRect(renderer_, &trunk);

        SDL_SetRenderDrawColor(renderer_, 120, 78, 46, 255);
        SDL_Rect trunk_highlight{
            trunk.x + trunk.w / 3,
            trunk.y,
            trunk.w / 4,
            trunk.h
        };
        SDL_RenderFillRect(renderer_, &trunk_highlight);

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

    DrawDecorativeApples(renderer_,
                         apple_texture_.First(),
                         apple_texture_.width > 0 ? apple_texture_.width : 18,
                         apple_texture_.height > 0 ? apple_texture_.height : 18,
                         trees,
                         camera_x_);

    for (const Vine& vine : vines_) {
        SDL_Rect rect = vine.rect;
        rect.x -= static_cast<int>(camera_x_);

        SDL_SetRenderDrawColor(renderer_, 34, 120, 44, 255);
        SDL_RenderFillRect(renderer_, &rect);

        SDL_SetRenderDrawColor(renderer_, 58, 160, 62, 255);
        SDL_Rect highlight{ rect.x + 2, rect.y, 2, rect.h };
        SDL_RenderFillRect(renderer_, &highlight);

        SDL_SetRenderDrawColor(renderer_, 48, 145, 52, 255);
        for (int y = rect.y + 10; y < rect.y + rect.h - 8; y += 18) {
            SDL_RenderDrawLine(renderer_, rect.x + 2, y, rect.x - 6, y + 4);
            SDL_RenderDrawLine(renderer_, rect.x + 3, y + 2, rect.x + 9, y + 6);
        }
    }

    for (const DeadlyVine& vine : deadly_vines_) {
        SDL_Rect rect = vine.rect;
        rect.x -= static_cast<int>(camera_x_);

        SDL_SetRenderDrawColor(renderer_, 122, 28, 74, 255);
        SDL_RenderFillRect(renderer_, &rect);

        SDL_SetRenderDrawColor(renderer_, 206, 52, 98, 255);
        SDL_Rect glow{rect.x + 2, rect.y, 3, rect.h};
        SDL_RenderFillRect(renderer_, &glow);

        SDL_SetRenderDrawColor(renderer_, 245, 222, 118, 255);
        for (int y = rect.y + 12; y < rect.y + rect.h - 8; y += 16) {
            SDL_RenderDrawLine(renderer_, rect.x + 1, y, rect.x - 8, y + 5);
            SDL_RenderDrawLine(renderer_, rect.x + rect.w - 1, y + 4, rect.x + rect.w + 8, y + 9);
        }
    }

    for (const Platform& platform : platforms_) {
        if (platform.rect.h >= 30) {
            continue;
        }

        SDL_Rect screen_rect = platform.rect;
        screen_rect.x -= static_cast<int>(camera_x_);

        SDL_SetRenderDrawColor(renderer_, 110, 72, 42, 255);
        SDL_RenderFillRect(renderer_, &screen_rect);

        SDL_SetRenderDrawColor(renderer_, 138, 94, 58, 255);
        SDL_Rect top_bark = screen_rect;
        top_bark.h = 5;
        SDL_RenderFillRect(renderer_, &top_bark);

        SDL_SetRenderDrawColor(renderer_, 42, 120, 45, 255);
        SDL_Rect moss{
            screen_rect.x + 8,
            screen_rect.y - 6,
            screen_rect.w - 16,
            6
        };
        SDL_RenderFillRect(renderer_, &moss);

        SDL_SetRenderDrawColor(renderer_, 58, 150, 58, 255);
        for (int gx = screen_rect.x + 10; gx < screen_rect.x + screen_rect.w - 10; gx += 12) {
            SDL_RenderDrawLine(renderer_, gx, screen_rect.y, gx + 2, screen_rect.y - 5);
            SDL_RenderDrawLine(renderer_, gx + 4, screen_rect.y, gx + 5, screen_rect.y - 6);
        }
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    for (const PoisonGasCloud& gas : poison_gas_) {
        SDL_Rect rect = gas.rect;
        rect.x -= static_cast<int>(camera_x_);

        SDL_SetRenderDrawColor(renderer_, 126, 224, 62, 92);
        SDL_Rect base{rect.x, rect.y + rect.h / 3, rect.w, (rect.h * 2) / 3};
        SDL_RenderFillRect(renderer_, &base);

        SDL_SetRenderDrawColor(renderer_, 178, 255, 86, 120);
        DrawFilledCircle(renderer_, rect.x + rect.w / 5, rect.y + rect.h / 2, rect.h / 2);
        DrawFilledCircle(renderer_, rect.x + rect.w / 2, rect.y + rect.h / 3, rect.h / 2);
        DrawFilledCircle(renderer_, rect.x + (rect.w * 4) / 5, rect.y + rect.h / 2, rect.h / 2);

        SDL_SetRenderDrawColor(renderer_, 92, 150, 44, 140);
        SDL_RenderDrawLine(renderer_, rect.x + 8, rect.y + rect.h - 5, rect.x + rect.w - 8, rect.y + rect.h - 5);
    }
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);

    for (const SpikeTrap& spike : spikes_) {
        SDL_Rect dest = spike.rect;
        dest.x -= static_cast<int>(camera_x_);

        if (!spike_texture_.Empty()) {
            SDL_RenderCopy(renderer_, spike_texture_.First(), nullptr, &dest);
        } else {
            SDL_SetRenderDrawColor(renderer_, 190, 190, 190, 255);
            SDL_RenderFillRect(renderer_, &dest);
        }
    }

    for (const SquirrelEnemy& squirrel : squirrels_) {
        squirrel.Render(renderer_, camera_x_);
    }

    player_.Render(renderer_, camera_x_);

    for (int i = 0; i < player_.GetMaxHealth(); ++i) {
        DrawHeart(renderer_, 20 + i * 36, 20, 4, i < player_.GetHealth());
    }

    SDL_RenderPresent(renderer_);
}

void Game::Shutdown() {
    DestroyTextureSet(player_texture_);
    DestroyTextureSet(idle_textures_);
    DestroyTextureSet(walk_textures_);
    DestroyTextureSet(punch_textures_);
    DestroyTextureSet(jump_textures_);
    DestroyTextureSet(heel_kick_textures_);
    DestroyTextureSet(background_texture_);
    DestroyTextureSet(apple_texture_);
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
