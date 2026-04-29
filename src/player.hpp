#pragma once
#include <SDL.h>
#include <vector>
#include "input.hpp"
#include "platform.hpp"
#include "texture_set.hpp"

class Player {
public:
    void SetPosition(float x, float y) { x_ = x; y_ = y; }
    void SetGroundY(float y) { ground_y_ = y; }
    float GetX() const { return x_; }
    float GetY() const { return y_; }
    int GetWidth() const { return base_texture_.width > 0 ? base_texture_.width : 48; }
    int GetHeight() const { return base_texture_.height > 0 ? base_texture_.height : 64; }

    void SetTexture(const TextureSet& texture_set);
    void SetIdleTextures(const TextureSet& textures);
    void SetWalkTextures(const TextureSet& textures);
    void SetPunchTextures(const TextureSet& textures);
    void SetJumpTextures(const TextureSet& textures);
    void SetHeelKickTextures(const TextureSet& textures);

    void CheckPlatformCollisions(const std::vector<Platform>& platforms);

    void Update(float dt, const InputState& input);
    void Render(SDL_Renderer* renderer, float camera_x) const;
    SDL_Rect GetBodyRect() const;
    SDL_Rect GetAttackRect() const;
    void ApplyKnockback(float vx, float vy);

    int GetHealth() const { return health_; }
    int GetMaxHealth() const { return max_health_; }
    void TakeDamage(int amount = 1);
    void Heal(int amount = 1);
    float GetVelocityY() const { return vy_; }
    void SetVelocityY(float v) { vy_ = v; }

private:
    float x_ = 0.0f;
    float y_ = 0.0f;
    float vx_ = 0.0f;
    float vy_ = 0.0f;
    float ground_y_ = 0.0f;
    bool on_ground_ = false;

    int health_ = 3;
    int max_health_ = 3;

    float punch_timer_ = 0.0f;
    float punch_frame_time_ = 0.0f;
    int punch_frame_ = 0;
    float heel_kick_timer_ = 0.0f;
    float heel_kick_frame_time_ = 0.0f;
    int heel_kick_frame_ = 0;

    TextureSet base_texture_{};
    TextureSet jump_textures_{};
    int jump_frame_ = 0;
    float jump_frame_time_ = 0.0f;
    TextureSet heel_kick_textures_{};
    TextureSet idle_textures_{};
    int idle_frame_ = 0;
    float idle_frame_time_ = 0.0f;
    bool idle_active_ = false;
    TextureSet walk_textures_{};
    int walk_frame_ = 0;
    float walk_frame_time_ = 0.0f;
    bool walk_active_ = false;
    TextureSet punch_textures_{};
    bool facing_left_ = false;
};
