#pragma once

#include <SDL.h>
#include <vector>

#include "texture_set.hpp"

class AbyssBoss {
public:
    void SetTextures(const TextureSet& head, const TextureSet& body, const TextureSet& tail);
    void Reset(float x, float ground_y);
    void Update(float dt, const SDL_Rect& player_rect);
    void Render(SDL_Renderer* renderer, float camera_x) const;

    bool TryTakeHit(const SDL_Rect& attack_rect);
    bool CheckContactHitPlayer(const SDL_Rect& player_rect, float* out_knockback_x) const;

    int GetHealth() const { return health_; }
    int GetMaxHealth() const { return max_health_; }
    bool IsDefeated() const { return health_ <= 0; }

private:
    enum class State {
        Loading,
        Charging,
        Leaving
    };

    struct Segment {
        float x = 0.0f;
        float y = 0.0f;
    };

    SDL_Rect GetSegmentRect(const Segment& segment, int width, int height) const;
    void ArrangeCoil();
    void ResetTrailToHead();
    void FollowTrail();
    float SegmentAngle(std::size_t index) const;

    std::vector<Segment> segments_{};
    TextureSet head_texture_{};
    TextureSet body_texture_{};
    TextureSet tail_texture_{};

    State state_ = State::Loading;
    float spawn_x_ = 0.0f;
    float ground_y_ = 0.0f;
    float state_timer_ = 0.0f;
    float hurt_cooldown_ = 0.0f;
    float pulse_time_ = 0.0f;
    float chase_speed_ = 0.0f;
    int health_ = 14;
    int max_health_ = 14;
    bool active_hitbox_ = true;
};
