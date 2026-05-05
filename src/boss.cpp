#include "boss.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr int kBodySegmentCount = 10;
constexpr int kTotalSegmentCount = kBodySegmentCount + 2;
constexpr int kFallbackHeadSize = 58;
constexpr int kFallbackBodySize = 46;
constexpr int kFallbackTailSize = 42;
constexpr float kSegmentSpacing = 34.0f;
constexpr float kLoadDuration = 5.0f;
constexpr float kChargeDuration = 3.6f;
constexpr float kLeaveDuration = 1.1f;
constexpr float kChargeSpeed = 200.0f;
constexpr float kLeaveSpeed = 520.0f;
constexpr float kHurtCooldown = 0.5f;
constexpr float kPi = 3.1415926535f;

float Length(float x, float y) {
    return std::max(std::sqrt(x * x + y * y), 0.001f);
}

void DrawFilledCircle(SDL_Renderer* renderer, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; ++dy) {
        const int dx_limit = static_cast<int>(std::sqrt(radius * radius - dy * dy));
        SDL_RenderDrawLine(renderer, cx - dx_limit, cy + dy, cx + dx_limit, cy + dy);
    }
}
}

void AbyssBoss::SetTextures(const TextureSet& head, const TextureSet& body, const TextureSet& tail) {
    head_texture_ = head;
    body_texture_ = body;
    tail_texture_ = tail;
}

void AbyssBoss::Reset(float x, float ground_y) {
    spawn_x_ = x;
    ground_y_ = ground_y;
    state_ = State::Loading;
    state_timer_ = 0.0f;
    hurt_cooldown_ = 0.0f;
    pulse_time_ = 0.0f;
    chase_speed_ = kChargeSpeed;
    health_ = max_health_;
    active_hitbox_ = true;

    segments_.assign(kTotalSegmentCount, Segment{});
    ArrangeCoil();
}

SDL_Rect AbyssBoss::GetSegmentRect(const Segment& segment, int width, int height) const {
    return SDL_Rect{
        static_cast<int>(segment.x) - width / 2,
        static_cast<int>(segment.y) - height / 2,
        width,
        height
    };
}

void AbyssBoss::ArrangeCoil() {
    if (segments_.empty()) {
        return;
    }

    const float center_x = spawn_x_;
    const float center_y = ground_y_ - 112.0f;
    const float coil_radius = 36.0f + std::sin(pulse_time_ * 6.0f) * 5.0f;

    segments_[0] = Segment{center_x + 4.0f, center_y - 44.0f};
    for (std::size_t i = 1; i < segments_.size(); ++i) {
        const float angle = pulse_time_ * 1.7f + static_cast<float>(i) * 0.82f;
        const float radius = coil_radius + static_cast<float>(i % 3) * 7.0f;
        segments_[i] = Segment{
            center_x + std::cos(angle) * radius,
            center_y + std::sin(angle) * radius * 0.68f
        };
    }
}

void AbyssBoss::ResetTrailToHead() {
    for (std::size_t i = 1; i < segments_.size(); ++i) {
        segments_[i] = Segment{
            segments_[0].x - static_cast<float>(i) * kSegmentSpacing,
            segments_[0].y
        };
    }
}

void AbyssBoss::FollowTrail() {
    for (std::size_t i = 1; i < segments_.size(); ++i) {
        Segment& previous = segments_[i - 1];
        Segment& current = segments_[i];
        float dx = current.x - previous.x;
        float dy = current.y - previous.y;
        const float distance = Length(dx, dy);

        if (distance <= kSegmentSpacing) {
            continue;
        }

        dx /= distance;
        dy /= distance;
        current.x = previous.x + dx * kSegmentSpacing;
        current.y = previous.y + dy * kSegmentSpacing;
    }
}

float AbyssBoss::SegmentAngle(std::size_t index) const {
    if (segments_.empty()) {
        return 0.0f;
    }

    const std::size_t target_index = index == 0 ? 1 : index - 1;
    if (target_index >= segments_.size()) {
        return 0.0f;
    }

    const Segment& current = segments_[index];
    const Segment& target = segments_[target_index];
    const float angle = std::atan2(target.y - current.y, target.x - current.x);
    return angle * 180.0f / kPi;
}

void AbyssBoss::Update(float dt, const SDL_Rect& player_rect) {
    if (health_ <= 0) {
        pulse_time_ += dt;
        return;
    }

    pulse_time_ += dt;
    state_timer_ += dt;
    if (hurt_cooldown_ > 0.0f) {
        hurt_cooldown_ = std::max(0.0f, hurt_cooldown_ - dt);
    }

    if (state_ == State::Loading) {
        active_hitbox_ = true;
        ArrangeCoil();

        if (state_timer_ >= kLoadDuration) {
            state_ = State::Charging;
            state_timer_ = 0.0f;
            ResetTrailToHead();
        }
        return;
    }

    if (state_ == State::Charging) {
        active_hitbox_ = true;
        const float target_x = static_cast<float>(player_rect.x + player_rect.w / 2);
        const float target_y = static_cast<float>(player_rect.y + player_rect.h / 2);
        float dx = target_x - segments_[0].x;
        float dy = target_y - segments_[0].y;
        const float distance = Length(dx, dy);
        dx /= distance;
        dy /= distance;

        segments_[0].x += dx * chase_speed_ * dt;
        segments_[0].y += dy * chase_speed_ * dt;
        FollowTrail();

        if (state_timer_ >= kChargeDuration) {
            state_ = State::Leaving;
            state_timer_ = 0.0f;
        }
        return;
    }

    active_hitbox_ = false;
    segments_[0].x += kLeaveSpeed * dt;
    segments_[0].y -= kLeaveSpeed * 0.14f * dt;
    FollowTrail();

    if (state_timer_ >= kLeaveDuration || segments_.back().x > 1180.0f) {
        state_ = State::Loading;
        state_timer_ = 0.0f;
        active_hitbox_ = true;
        ArrangeCoil();
    }
}

bool AbyssBoss::TryTakeHit(const SDL_Rect& attack_rect) {
    if (health_ <= 0 || hurt_cooldown_ > 0.0f || attack_rect.w <= 0 || attack_rect.h <= 0 || !active_hitbox_) {
        return false;
    }

    const int head_w = head_texture_.width > 0 ? head_texture_.width : kFallbackHeadSize;
    const int head_h = head_texture_.height > 0 ? head_texture_.height : kFallbackHeadSize;
    const int body_w = body_texture_.width > 0 ? body_texture_.width : kFallbackBodySize;
    const int body_h = body_texture_.height > 0 ? body_texture_.height : kFallbackBodySize;
    const int tail_w = tail_texture_.width > 0 ? tail_texture_.width : kFallbackTailSize;
    const int tail_h = tail_texture_.height > 0 ? tail_texture_.height : kFallbackTailSize;

    for (std::size_t i = 0; i < segments_.size(); ++i) {
        const bool is_head = i == 0;
        const bool is_tail = i + 1 == segments_.size();
        SDL_Rect hitbox = GetSegmentRect(
            segments_[i],
            is_head ? head_w : (is_tail ? tail_w : body_w),
            is_head ? head_h : (is_tail ? tail_h : body_h));

        hitbox.x += 6;
        hitbox.y += 6;
        hitbox.w -= 12;
        hitbox.h -= 12;
        if (SDL_HasIntersection(&hitbox, &attack_rect) == SDL_TRUE) {
            --health_;
            hurt_cooldown_ = kHurtCooldown;
            return true;
        }
    }

    return false;
}

bool AbyssBoss::CheckContactHitPlayer(const SDL_Rect& player_rect, float* out_knockback_x) const {
    if (health_ <= 0 || state_ != State::Charging || !active_hitbox_) {
        return false;
    }

    const int head_w = head_texture_.width > 0 ? head_texture_.width : kFallbackHeadSize;
    const int head_h = head_texture_.height > 0 ? head_texture_.height : kFallbackHeadSize;
    const int body_w = body_texture_.width > 0 ? body_texture_.width : kFallbackBodySize;
    const int body_h = body_texture_.height > 0 ? body_texture_.height : kFallbackBodySize;

    const std::size_t damaging_segments = std::min<std::size_t>(segments_.size(), 5);
    for (std::size_t i = 0; i < damaging_segments; ++i) {
        SDL_Rect hitbox = GetSegmentRect(segments_[i], i == 0 ? head_w : body_w, i == 0 ? head_h : body_h);
        hitbox.x += 8;
        hitbox.y += 8;
        hitbox.w -= 16;
        hitbox.h -= 16;

        if (SDL_HasIntersection(&hitbox, &player_rect) == SDL_TRUE) {
            if (out_knockback_x) {
                const int player_center = player_rect.x + player_rect.w / 2;
                *out_knockback_x = player_center < static_cast<int>(segments_[i].x) ? -220.0f : 220.0f;
            }
            return true;
        }
    }

    return false;
}

void AbyssBoss::Render(SDL_Renderer* renderer, float camera_x) const {
    if (segments_.empty()) {
        return;
    }

    if (health_ <= 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 92, 48, 124, 90);
        DrawFilledCircle(renderer,
                         static_cast<int>(segments_[0].x - camera_x),
                         static_cast<int>(segments_[0].y),
                         52);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        return;
    }

    const bool hurt = hurt_cooldown_ > 0.0f;
    const int head_w = head_texture_.width > 0 ? head_texture_.width : kFallbackHeadSize;
    const int head_h = head_texture_.height > 0 ? head_texture_.height : kFallbackHeadSize;
    const int body_w = body_texture_.width > 0 ? body_texture_.width : kFallbackBodySize;
    const int body_h = body_texture_.height > 0 ? body_texture_.height : kFallbackBodySize;
    const int tail_w = tail_texture_.width > 0 ? tail_texture_.width : kFallbackTailSize;
    const int tail_h = tail_texture_.height > 0 ? tail_texture_.height : kFallbackTailSize;

    if (state_ == State::Loading) {
        const int pulse = static_cast<int>((std::sin(pulse_time_ * 7.0f) + 1.0f) * 12.0f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 110, 44, 170, 70);
        DrawFilledCircle(renderer,
                         static_cast<int>(spawn_x_ - camera_x),
                         static_cast<int>(ground_y_ - 112.0f),
                         68 + pulse);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    for (std::size_t reverse_i = segments_.size(); reverse_i > 0; --reverse_i) {
        const std::size_t i = reverse_i - 1;
        const bool is_head = i == 0;
        const bool is_tail = i + 1 == segments_.size();
        const TextureSet& texture = is_head ? head_texture_ : (is_tail ? tail_texture_ : body_texture_);
        const int draw_w = is_head ? head_w : (is_tail ? tail_w : body_w);
        const int draw_h = is_head ? head_h : (is_tail ? tail_h : body_h);
        SDL_Rect dest = GetSegmentRect(segments_[i], draw_w, draw_h);
        dest.x -= static_cast<int>(camera_x);

        SDL_Texture* frame = texture.First();
        if (frame) {
            if (hurt) {
                SDL_SetTextureColorMod(frame, 220, 80, 120);
            }
            const double angle = SegmentAngle(i) + (is_tail ? 180.0 : 0.0);
            SDL_RenderCopyEx(renderer, frame, nullptr, &dest, angle, nullptr, SDL_FLIP_NONE);
            SDL_SetTextureColorMod(frame, 255, 255, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, is_head ? 116 : 70, is_head ? 190 : 130, is_head ? 70 : 52, 255);
            SDL_RenderFillRect(renderer, &dest);
        }
    }
}
