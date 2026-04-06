#pragma once

#include <SDL.h>
#include <vector>
#include "texture_set.hpp"

struct AcornProjectile {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    bool active = false;
};

class SquirrelEnemy {
public:
    void SetPosition(float x, float y);
    void SetTextures(const TextureSet& squirrel_textures, const TextureSet& acorn_textures);
    void Update(float dt, const SDL_Rect& player_rect);
    void Render(SDL_Renderer* renderer, float camera_x) const;
    bool TryTakeHit(const SDL_Rect& attack_rect);
    bool CheckProjectileHitPlayer(const SDL_Rect& player_rect, float* out_knockback_x) ;
    bool IsActive() const { return hits_remaining_ > 0; }

private:
    SDL_Rect GetBodyRect() const;

    float x_ = 0.0f;
    float y_ = 0.0f;
    float shot_timer_ = 0.9f;
    float hurt_cooldown_ = 0.0f;
    int hits_remaining_ = 2;
    std::vector<AcornProjectile> acorns_{};
    TextureSet squirrel_textures_{};
    TextureSet acorn_textures_{};
};
