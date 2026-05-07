#pragma once
#include <SDL.h>
#include <vector>
#include "boss.hpp"
#include "enemy.hpp"
#include "input.hpp"
#include "platform.hpp"
#include "player.hpp"
#include "texture_set.hpp"

struct Vine {
    SDL_Rect rect{};
};

struct SpikeTrap {
    SDL_Rect rect{};
};

struct PoisonGasCloud {
    SDL_Rect rect{};
};

struct DeadlyVine {
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
    void ResetRun();

    // Boss arena helpers. The main Update/Render functions hand off to these
    // after the player enters the cave portal.
    void EnterBossArena();
    void UpdateBossArena(float dt);
    void RenderBossArena();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    TextureSet player_texture_{};
    TextureSet idle_textures_{};
    TextureSet walk_textures_{};
    TextureSet punch_textures_{};
    TextureSet jump_textures_{};
    TextureSet heel_kick_textures_{};
    TextureSet background_texture_{};
    TextureSet apple_texture_{};
    TextureSet squirrel_textures_{};
    TextureSet acorn_textures_{};
    TextureSet spike_texture_{};

    // Snake boss textures loaded by Game and reused by AbyssBoss.
    TextureSet snake_head_texture_{};
    TextureSet snake_body_texture_{};
    TextureSet snake_tail_texture_{};

    std::vector<Platform> platforms_{};
    std::vector<Platform> forest_platforms_{};

    // Separate arena layout used after the portal transition.
    std::vector<Platform> boss_platforms_{};
    std::vector<SquirrelEnemy> squirrels_{};
    std::vector<SpikeTrap> spikes_{};
    std::vector<Vine> vines_{};
    std::vector<PoisonGasCloud> poison_gas_{};
    std::vector<DeadlyVine> deadly_vines_{};

    // Forest-level portal rectangle that sends the player into the boss arena.
    SDL_Rect cave_portal_{};
    bool running_ = false;

    // Chooses whether Update/Render use the forest level or boss arena.
    bool in_boss_arena_ = false;
    float camera_x_ = 0.0f;
    float player_damage_cooldown_ = 0.0f;
    float thorn_vine_damage_timer_ = 0.0f;

    InputState input_{};
    Player player_{};

    // The snake boss object; it handles its own health, movement, and drawing.
    AbyssBoss boss_{};
};
