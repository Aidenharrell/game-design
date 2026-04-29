#pragma once
#include <SDL.h>
#include <vector>
#include "input.hpp"
#include "player.hpp"
#include "platform.hpp"
#include "texture_set.hpp"

struct SquirrelEnemy {
    float x = 0.0f;
    float y = 0.0f;
    float shoot_timer = 0.0f;
    float shoot_anim_timer = 0.0f;
    int shoot_frame = 0;
    bool facing_left = true;
    bool shooting = false;
};

struct Vine {
    SDL_Rect rect{};
};

struct AcornProjectile {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float anim_timer = 0.0f;
    int anim_frame = 0;
    bool active = true;
};

struct SpikeTrap {
    SDL_Rect rect{};
};

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

    TextureSet player_texture_{};
    TextureSet idle_textures_{};
    TextureSet walk_textures_{};
    TextureSet punch_textures_{};
    TextureSet jump_textures_{};
    TextureSet heel_kick_textures_{};

    TextureSet squirrel_textures_{};
    TextureSet acorn_textures_{};
    TextureSet spike_texture_{};

    std::vector<Platform> platforms_;
    std::vector<SquirrelEnemy> squirrels_;
    std::vector<AcornProjectile> acorns_;
    std::vector<SpikeTrap> spikes_;
    std::vector<Vine> vines_;

    bool running_ = false;

    float camera_x_ = 0.0f;
    float player_damage_cooldown_ = 0.0f;

    InputState input_{};
    Player player_{};
};