#include "enemy.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr int kSquirrelWidth = 44;
constexpr int kSquirrelHeight = 36;
constexpr float kShootCooldown = 2.2f;
constexpr float kRageShootCooldown = 1.15f;
constexpr float kHurtCooldown = 0.3f;
constexpr float kSquirrelFrameDuration = 0.08f;
constexpr float kAcornSpeed = 220.0f;
constexpr float kRageAcornSpeed = 390.0f;
constexpr float kAcornGravity = 230.0f;
constexpr float kRageAcornGravity = 260.0f;
constexpr int kAcornSize = 12;
constexpr float kAcornFrameDuration = 0.07f;
constexpr float kAcornDespawnMaxX = 10400.0f;
constexpr float kSquirrelKnockbackSpeed = 820.0f;
constexpr float kSquirrelKnockbackUpSpeed = -360.0f;
constexpr float kSquirrelKnockbackGravity = 900.0f;
constexpr float kSquirrelKnockbackDrag = 260.0f;
}

void SquirrelEnemy::SetPosition(float x, float y) {
    x_ = x;
    y_ = y;
}

void SquirrelEnemy::SetSquirrelTextures(const TextureSet& textures) {
    squirrel_textures_ = textures;
}

void SquirrelEnemy::SetAcornTextures(const TextureSet& textures) {
    acorn_textures_ = textures;
}

SDL_Rect SquirrelEnemy::GetBodyRect() const {
    const int width = squirrel_textures_.width > 0 ? squirrel_textures_.width : kSquirrelWidth;
    const int height = squirrel_textures_.height > 0 ? squirrel_textures_.height : kSquirrelHeight;

    return SDL_Rect{
        static_cast<int>(x_),
        static_cast<int>(y_) - height,
        width,
        height
    };
}

void SquirrelEnemy::Update(float dt, const SDL_Rect& player_rect) {
    if (std::fabs(knockback_vx_) > 1.0f || std::fabs(knockback_vy_) > 1.0f) {
        x_ += knockback_vx_ * dt;
        y_ += knockback_vy_ * dt;
        knockback_vy_ += kSquirrelKnockbackGravity * dt;

        if (knockback_vx_ > 0.0f) {
            knockback_vx_ = std::max(0.0f, knockback_vx_ - kSquirrelKnockbackDrag * dt);
        } else {
            knockback_vx_ = std::min(0.0f, knockback_vx_ + kSquirrelKnockbackDrag * dt);
        }
    } else {
        knockback_vx_ = 0.0f;
        knockback_vy_ = 0.0f;
    }

    if (hurt_cooldown_ > 0.0f) {
        hurt_cooldown_ = std::max(0.0f, hurt_cooldown_ - dt);
    }

    if (hits_remaining_ > 0 && !squirrel_textures_.Empty()) {
        animation_time_ += dt;
        const float cycle = kSquirrelFrameDuration * static_cast<float>(squirrel_textures_.frames.size());
        if (animation_time_ >= cycle) {
            animation_time_ = std::fmod(animation_time_, cycle);
        }
    }

    if (hits_remaining_ > 0) {
        shot_timer_ -= dt;
        facing_left_ = player_rect.x > static_cast<int>(x_);
        if (shot_timer_ <= 0.0f) {
            const float acorn_speed = raging_ ? kRageAcornSpeed : kAcornSpeed;
            shot_timer_ = raging_ ? kRageShootCooldown : kShootCooldown;

            const SDL_Rect body_rect = GetBodyRect();
            const float start_x = x_ + (body_rect.w * 0.5f);
            const float start_y = y_ - (body_rect.h * 0.65f);
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
            projectile.vx = dx * acorn_speed;
            projectile.vy = dy * acorn_speed - (raging_ ? 12.0f : 30.0f);
            projectile.active = true;
            acorns_.push_back(projectile);
        }
    }

    for (AcornProjectile& acorn : acorns_) {
        if (!acorn.active) {
            continue;
        }

        if (!acorn_textures_.Empty()) {
            acorn.frame_time += dt;
            if (acorn.frame_time >= kAcornFrameDuration) {
                acorn.frame_time = 0.0f;
                acorn.frame_index = (acorn.frame_index + 1) % static_cast<int>(acorn_textures_.frames.size());
            }
        }

        acorn.vy += (raging_ ? kRageAcornGravity : kAcornGravity) * dt;
        acorn.x += acorn.vx * dt;
        acorn.y += acorn.vy * dt;

        if (acorn.y > 900.0f || acorn.x < -400.0f || acorn.x > kAcornDespawnMaxX) {
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

    hits_remaining_ = 0;
    const int attack_center = attack_rect.x + attack_rect.w / 2;
    const int enemy_center = enemy_rect.x + enemy_rect.w / 2;
    knockback_vx_ = enemy_center >= attack_center ? kSquirrelKnockbackSpeed : -kSquirrelKnockbackSpeed;
    knockback_vy_ = kSquirrelKnockbackUpSpeed;
    x_ += knockback_vx_ > 0.0f ? 14.0f : -14.0f;
    acorns_.clear();
    hurt_cooldown_ = kHurtCooldown;
    return true;
}

bool SquirrelEnemy::CheckProjectileHitPlayer(const SDL_Rect& player_rect, float* out_knockback_x) {
    for (AcornProjectile& acorn : acorns_) {
        if (!acorn.active) {
            continue;
        }

        SDL_Rect acorn_rect{
            static_cast<int>(acorn.x) - ((acorn_textures_.width > 0 ? acorn_textures_.width : kAcornSize) / 2),
            static_cast<int>(acorn.y) - ((acorn_textures_.height > 0 ? acorn_textures_.height : kAcornSize) / 2),
            acorn_textures_.width > 0 ? acorn_textures_.width : kAcornSize,
            acorn_textures_.height > 0 ? acorn_textures_.height : kAcornSize
        };
        acorn_rect.x += 4;
        acorn_rect.y += 4;
        acorn_rect.w = std::max(1, acorn_rect.w - 8);
        acorn_rect.h = std::max(1, acorn_rect.h - 8);

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

    SDL_Texture* squirrel_texture = squirrel_textures_.First();
    if (!squirrel_textures_.Empty()) {
        const int frame_count = static_cast<int>(squirrel_textures_.frames.size());
        int frame_index = 0;
        if (hits_remaining_ > 0) {
            frame_index = static_cast<int>(animation_time_ / kSquirrelFrameDuration) % frame_count;
        }
        squirrel_texture = squirrel_textures_.frames[frame_index];
    }

    if (squirrel_texture) {
        if (hits_remaining_ <= 0) {
            SDL_SetTextureColorMod(squirrel_texture, 110, 110, 110);
        } else if (raging_) {
            SDL_SetTextureColorMod(squirrel_texture, 255, 92, 92);
        }

        const SDL_RendererFlip flip = facing_left_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderCopyEx(renderer, squirrel_texture, nullptr, &body, 0.0, nullptr, flip);
        SDL_SetTextureColorMod(squirrel_texture, 255, 255, 255);
    } else {
        if (raging_ && hits_remaining_ > 0) {
            SDL_SetRenderDrawColor(renderer, 190, 54, 50, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 150, 92, 48, 255);
        }
        SDL_RenderFillRect(renderer, &body);
    }

    for (const AcornProjectile& acorn : acorns_) {
        if (!acorn.active) {
            continue;
        }

        const int draw_w = acorn_textures_.width > 0 ? acorn_textures_.width : kAcornSize;
        const int draw_h = acorn_textures_.height > 0 ? acorn_textures_.height : kAcornSize;
        SDL_Rect acorn_rect{
            static_cast<int>(acorn.x - camera_x) - (draw_w / 2),
            static_cast<int>(acorn.y) - (draw_h / 2),
            draw_w,
            draw_h
        };

        if (!acorn_textures_.Empty()) {
            SDL_Texture* acorn_texture = acorn_textures_.frames[acorn.frame_index];
            if (raging_) {
                SDL_SetTextureColorMod(acorn_texture, 255, 86, 42);
            }
            SDL_RenderCopy(renderer, acorn_texture, nullptr, &acorn_rect);
            if (raging_) {
                SDL_SetTextureColorMod(acorn_texture, 255, 255, 255);
            }
        } else {
            if (raging_) {
                SDL_SetRenderDrawColor(renderer, 235, 56, 36, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 122, 75, 34, 255);
            }
            SDL_RenderFillRect(renderer, &acorn_rect);
        }
    }
}
