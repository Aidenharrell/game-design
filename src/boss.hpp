#pragma once

#include <SDL.h>
#include <vector>

#include "texture_set.hpp"

// AbyssBoss controls the snake boss fight.
// It owns the snake's attack state, health, segment positions, and textures.
class AbyssBoss {
public:
    // Stores the already-loaded snake textures from Game.
    // The boss does not create or destroy SDL textures; Game owns that cleanup.
    void SetTextures(const TextureSet& head, const TextureSet& body, const TextureSet& tail);

    // Starts the boss from a fresh state at its arena spawn point.
    void Reset(float x, float ground_y);

    // Advances the boss behavior for one frame.
    // player_rect is used so the charging snake can chase the player.
    void Update(float dt, const SDL_Rect& player_rect);

    // Draws the snake using camera_x so it scrolls with the arena camera.
    void Render(SDL_Renderer* renderer, float camera_x) const;

    // Returns true when the player's attack rectangle hits a vulnerable segment.
    bool TryTakeHit(const SDL_Rect& attack_rect);

    // Returns true when the charging snake touches the player.
    bool CheckContactHitPlayer(const SDL_Rect& player_rect, float* out_knockback_x) const;

    int GetHealth() const { return health_; }
    int GetMaxHealth() const { return max_health_; }
    bool IsDefeated() const { return health_ <= 0; }

private:
    // Loading: coiled up and charging its attack.
    // Charging: head chases the player and the body follows.
    // Leaving: snake exits the screen before beginning the next load cycle.
    enum class State {
        Loading,
        Charging,
        Leaving
    };

    // One drawable piece of the snake.
    // Total count is decided in boss.cpp: head, body pieces, and tail.
    struct Segment {
        float x = 0.0f;
        float y = 0.0f;
    };

    // Builds a centered hit/draw rectangle from a segment's center position.
    SDL_Rect GetSegmentRect(const Segment& segment, int width, int height) const;

    // Places all snake segments into a coiled shape during the load phase.
    void ArrangeCoil();

    // Resets the body behind the head at the start of a charge.
    void ResetTrailToHead();

    // Moves each body part toward the part in front of it.
    void FollowTrail();

    // Returns the draw angle for a segment so its texture points along the chain.
    float SegmentAngle(std::size_t index) const;

    // Segment 0 is the head, the middle segments are body pieces, and the last is tail.
    std::vector<Segment> segments_{};

    // These TextureSets point to textures owned and destroyed by Game.
    TextureSet head_texture_{};
    TextureSet body_texture_{};
    TextureSet tail_texture_{};

    // Current attack phase.
    State state_ = State::Loading;

    // Where the snake reforms when it starts loading again.
    float spawn_x_ = 0.0f;

    // Bottom of the arena floor; used to position the coil.
    float ground_y_ = 0.0f;

    // Timers drive phase changes, animation pulses, and damage cooldowns.
    float state_timer_ = 0.0f;
    float hurt_cooldown_ = 0.0f;
    float pulse_time_ = 0.0f;

    // Speed used while the head chases the player.
    float chase_speed_ = 0.0f;

    int health_ = 12;
    int max_health_ = 12;

    // Disabled while the snake is leaving the screen.
    bool active_hitbox_ = true;
};
