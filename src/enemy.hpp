#pragma once

#include <SDL.h>
#include <vector>
#include "texture_set.hpp"

struct AcornProjectile {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float frame_time = 0.0f;
    int frame_index = 0;
    bool active = false;
};

class SquirrelEnemy {
public:
    void SetPosition(float x, float y);
    void SetSquirrelTextures(const TextureSet& textures);
    void SetAcornTextures(const TextureSet& textures);
    void SetRaging(bool raging) { raging_ = raging; }
    void Update(float dt, const SDL_Rect& player_rect);
    void Render(SDL_Renderer* renderer, float camera_x) const;
    bool TryTakeHit(const SDL_Rect& attack_rect);
    bool CheckProjectileHitPlayer(const SDL_Rect& player_rect, float* out_knockback_x);
    bool IsActive() const { return hits_remaining_ > 0; }

private:
    SDL_Rect GetBodyRect() const;

    float x_ = 0.0f;
    float y_ = 0.0f;
    float knockback_vx_ = 0.0f;
    float knockback_vy_ = 0.0f;
    float shot_timer_ = 0.9f;
    float hurt_cooldown_ = 0.0f;
    float animation_time_ = 0.0f;
    int hits_remaining_ = 2;
    bool facing_left_ = false;
    bool raging_ = false;
    TextureSet squirrel_textures_{};
    TextureSet acorn_textures_{};
    std::vector<AcornProjectile> acorns_{};
};
