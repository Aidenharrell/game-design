#include "enemy.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr int kSquirrelWidth = 44;
constexpr int kSquirrelHeight = 36;
constexpr float kShootCooldown = 1.6f;
constexpr float kHurtCooldown = 0.3f;
constexpr float kAcornSpeed = 280.0f;
constexpr float kAcornGravity = 260.0f;
constexpr int kAcornSize = 12;
constexpr float kSquirrelAnimFps = 10.0f;
constexpr float kAcornSpinFps = 12.0f;
}

void SquirrelEnemy::SetPosition(float x, float y) {
    x_ = x;
    y_ = y;
}

void SquirrelEnemy::SetTextures(const TextureSet& squirrel_textures, const TextureSet& acorn_textures) {
    squirrel_textures_ = squirrel_textures;
    acorn_textures_ = acorn_textures;
}

SDL_Rect SquirrelEnemy::GetBodyRect() const {
    return SDL_Rect{
        static_cast<int>(x_),
        static_cast<int>(y_) - kSquirrelHeight,
        kSquirrelWidth,
        kSquirrelHeight
    };
}

void SquirrelEnemy::Update(float dt, const SDL_Rect& player_rect) {
    if (hurt_cooldown_ > 0.0f) {
        hurt_cooldown_ = std::max(0.0f, hurt_cooldown_ - dt);
    }

    if (hits_remaining_ > 0) {
        shot_timer_ -= dt;
        if (shot_timer_ <= 0.0f) {
            shot_timer_ = kShootCooldown;

            const float start_x = x_ + (kSquirrelWidth * 0.5f);
            const float start_y = y_ - (kSquirrelHeight * 0.65f);
            const float target_x = static_cast<float>(player_rect.x + player_rect.w / 2);
            const float target_y = static_cast<float>(player_rect.y + player_rect.h / 2);
            float dx = target_x - start_x;
            float dy = target_y - start_y;
            const float length = std::max(std::sqrt(dx * dx + dy * dy), 1.0f);
            dx /= length;
            dy /= length;

            AcornProjectile projectile;
            projectile.x = start_x;
            projectile.y = start_y;
            projectile.vx = dx * kAcornSpeed;
            projectile.vy = dy * kAcornSpeed - 30.0f;
            projectile.active = true;
            acorns_.push_back(projectile);
        }
    }

    for (AcornProjectile& acorn : acorns_) {
        if (!acorn.active) {
            continue;
        }

        acorn.vy += kAcornGravity * dt;
        acorn.x += acorn.vx * dt;
        acorn.y += acorn.vy * dt;

        if (acorn.y > 900.0f || acorn.x < -400.0f || acorn.x > 6000.0f) {
            acorn.active = false;
        }
    }

    acorns_.erase(
        std::remove_if(acorns_.begin(), acorns_.end(),
                       [](const AcornProjectile& acorn) { return !acorn.active; }),
        acorns_.end());
}

bool SquirrelEnemy::TryTakeHit(const SDL_Rect& attack_rect) {
    if (hits_remaining_ <= 0 || hurt_cooldown_ > 0.0f || attack_rect.w <= 0 || attack_rect.h <= 0) {
        return false;
    }

    SDL_Rect enemy_rect = GetBodyRect();
    if (!SDL_HasIntersection(&enemy_rect, &attack_rect)) {
        return false;
    }

    --hits_remaining_;
    hurt_cooldown_ = kHurtCooldown;
    return true;
}

bool SquirrelEnemy::CheckProjectileHitPlayer(const SDL_Rect& player_rect, float* out_knockback_x) {
    for (AcornProjectile& acorn : acorns_) {
        if (!acorn.active) {
            continue;
        }

        SDL_Rect acorn_rect{
            static_cast<int>(acorn.x) - (kAcornSize / 2),
            static_cast<int>(acorn.y) - (kAcornSize / 2),
            kAcornSize,
            kAcornSize
        };

        if (SDL_HasIntersection(&acorn_rect, &player_rect)) {
            acorn.active = false;
            if (out_knockback_x) {
                *out_knockback_x = acorn.vx >= 0.0f ? 180.0f : -180.0f;
            }
            return true;
        }
    }

    return false;
}

void SquirrelEnemy::Render(SDL_Renderer* renderer, float camera_x) const {
    SDL_Rect body = GetBodyRect();
    body.x -= static_cast<int>(camera_x);

    if (!squirrel_textures_.Empty()) {
        const Uint32 ticks = SDL_GetTicks();
        const std::size_t frame_count = squirrel_textures_.frames.size();
        const std::size_t frame_index =
            static_cast<std::size_t>((ticks * kSquirrelAnimFps) / 1000.0f) % frame_count;
        SDL_Texture* squirrel_texture = squirrel_textures_.frames[frame_index];
        SDL_SetTextureColorMod(
            squirrel_texture,
            hits_remaining_ > 0 ? 255 : 110,
            hits_remaining_ > 0 ? 255 : 110,
            hits_remaining_ > 0 ? 255 : 110);
        SDL_RenderCopy(renderer, squirrel_texture, nullptr, &body);
        SDL_SetTextureColorMod(squirrel_texture, 255, 255, 255);
    } else {
        if (hits_remaining_ > 0) {
            SDL_SetRenderDrawColor(renderer, 150, 92, 48, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
        }
        SDL_RenderFillRect(renderer, &body);

        SDL_SetRenderDrawColor(renderer, 225, 210, 185, 255);
        SDL_Rect belly{
            body.x + 8,
            body.y + 10,
            body.w - 16,
            body.h - 12
        };
        SDL_RenderFillRect(renderer, &belly);

        SDL_SetRenderDrawColor(renderer, 110, 60, 32, 255);
        SDL_Rect tail{
            body.x + body.w - 4,
            body.y - 6,
            16,
            28
        };
        SDL_RenderFillRect(renderer, &tail);
    }

    for (const AcornProjectile& acorn : acorns_) {
        if (!acorn.active) {
            continue;
        }

        SDL_Rect acorn_rect{
            static_cast<int>(acorn.x - camera_x) - (kAcornSize / 2),
            static_cast<int>(acorn.y) - (kAcornSize / 2),
            kAcornSize,
            kAcornSize
        };

        if (!acorn_textures_.Empty()) {
            const Uint32 ticks = SDL_GetTicks();
            const std::size_t frame_count = acorn_textures_.frames.size();
            const std::size_t frame_index =
                static_cast<std::size_t>((ticks * kAcornSpinFps) / 1000.0f) % frame_count;
            SDL_RenderCopy(renderer, acorn_textures_.frames[frame_index], nullptr, &acorn_rect);
        } else {
            SDL_SetRenderDrawColor(renderer, 122, 75, 34, 255);
            SDL_RenderFillRect(renderer, &acorn_rect);
        }
    }
}
