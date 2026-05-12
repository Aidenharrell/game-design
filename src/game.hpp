#pragma once
#include <SDL.h>
#include <vector>
#include "boss.hpp"
#include "enemy.hpp"
#include "input.hpp"
#include "platform.hpp"
#include "player.hpp"
#include "texture_set.hpp"
#include <SDL_ttf.h>

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
    void RespawnAtRageCheckpoint();

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

    bool in_menu_ = false;
    bool paused_ = false;
    TTF_Font* menu_font_ = nullptr;
    SDL_Texture* level1_text_ = nullptr;
    SDL_Texture* quit_text_ = nullptr;
    SDL_Texture* pause_text_ = nullptr;
    SDL_Texture* home_text_ = nullptr;
    SDL_Texture* retry_text_ = nullptr;
    SDL_Texture* play_again_text_ = nullptr;
    SDL_Texture* death_text_ = nullptr;
    SDL_Texture* win_text_ = nullptr;
    SDL_Texture* enter_boss_text_ = nullptr;
    SDL_Rect level1_button_ = {350, 200, 260, 60};
    SDL_Rect quit_button_ = {350, 300, 260, 60};
    SDL_Rect pause_button_ = {350, 220, 260, 80};
    SDL_Rect home_button_ = {0, 0, 0, 0};
    int score_ = 0;
    int max_x_reached_ = 0;
    SDL_Texture* title_text_ = nullptr;
    bool has_died_ = false;
    bool last_run_died_ = false;
    bool last_run_won_ = false;
    int high_score_ = 0;
    int final_score_ = 0;
    bool rage_checkpoint_unlocked_ = false;

    // Chooses whether Update/Render use the forest level or boss arena.
    bool in_boss_arena_ = false;
    float camera_x_ = 0.0f;
    float player_damage_cooldown_ = 0.0f;
    float player_damage_flash_timer_ = 0.0f;
    float thorn_vine_damage_timer_ = 0.0f;
    float boss_hit_feedback_timer_ = 0.0f;

    InputState input_{};
    Player player_{};

    // The snake boss object; it handles its own health, movement, and drawing.
    AbyssBoss boss_{};
};
