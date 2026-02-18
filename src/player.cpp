#include "player.hpp"
#include <algorithm>
#include <cmath>

static const float kMoveSpeed = 260.0f;
static const float kJumpVelocity = -520.0f;
static const float kGravity = 1400.0f;
static const float kPunchDuration = 0.18f;
static const float kIdleFrameDuration = 0.12f;
static const float kWalkFrameDuration = 0.10f;

void Player::SetTexture(SDL_Texture* texture, int w, int h) {
    texture_ = texture;
    base_tex_w_ = w;
    base_tex_h_ = h;
}

void Player::SetIdleTextures(const std::vector<SDL_Texture*>& textures, int w, int h) {
    idle_textures_ = textures;
    idle_tex_w_ = w;
    idle_tex_h_ = h;
    idle_frame_ = 0;
    idle_frame_time_ = 0.0f;
}

void Player::SetWalkTextures(const std::vector<SDL_Texture*>& textures, int w, int h) {
    walk_textures_ = textures;
    walk_tex_w_ = w;
    walk_tex_h_ = h;
    walk_frame_ = 0;
    walk_frame_time_ = 0.0f;
}

void Player::Update(float dt, const InputState& input) {
    float move = 0.0f;
    if (input.move_left) move -= 1.0f;
    if (input.move_right) move += 1.0f;
    vx_ = move * kMoveSpeed;

    if (vx_ < 0.0f) {
        facing_left_ = true;
    } else if (vx_ > 0.0f) {
        facing_left_ = false;
    }

    if (on_ground_ && input.jump_pressed) {
        vy_ = kJumpVelocity;
        on_ground_ = false;
    }

    if (input.punch_pressed) {
        punch_timer_ = kPunchDuration;
    }

    vy_ += kGravity * dt;
    x_ += vx_ * dt;
    y_ += vy_ * dt;

    if (y_ >= ground_y_) {
        y_ = ground_y_;
        vy_ = 0.0f;
        on_ground_ = true;
    }

    if (punch_timer_ > 0.0f) {
        punch_timer_ -= dt;
        if (punch_timer_ < 0.0f) punch_timer_ = 0.0f;
    }

    walk_active_ = on_ground_ && std::fabs(vx_) >= 0.01f && punch_timer_ <= 0.0f;
    if (walk_active_ && !walk_textures_.empty()) {
        walk_frame_time_ += dt;
        if (walk_frame_time_ >= kWalkFrameDuration) {
            walk_frame_time_ = 0.0f;
            walk_frame_ = (walk_frame_ + 1) % static_cast<int>(walk_textures_.size());
        }
    } else {
        walk_frame_ = 0;
        walk_frame_time_ = 0.0f;
    }

    idle_active_ = on_ground_ && std::fabs(vx_) < 0.01f && punch_timer_ <= 0.0f;
    if (idle_active_ && !idle_textures_.empty()) {
        idle_frame_time_ += dt;
        if (idle_frame_time_ >= kIdleFrameDuration) {
            idle_frame_time_ = 0.0f;
            idle_frame_ = (idle_frame_ + 1) % static_cast<int>(idle_textures_.size());
        }
    } else {
        idle_frame_ = 0;
        idle_frame_time_ = 0.0f;
    }
}

void Player::Render(SDL_Renderer* renderer) const {
    int draw_w = base_tex_w_;
    int draw_h = base_tex_h_;
    if (draw_w <= 0) draw_w = 48;
    if (draw_h <= 0) draw_h = 64;

    SDL_Texture* render_texture = texture_;
    if (walk_active_ && !walk_textures_.empty()) {
        render_texture = walk_textures_[walk_frame_];
        draw_w = walk_tex_w_;
        draw_h = walk_tex_h_;
    } else if (idle_active_ && !idle_textures_.empty()) {
        render_texture = idle_textures_[idle_frame_];
        draw_w = idle_tex_w_;
        draw_h = idle_tex_h_;
    }

    SDL_Rect body{static_cast<int>(x_), static_cast<int>(y_) - draw_h, draw_w, draw_h};

    if (render_texture) {
        const SDL_RendererFlip flip = facing_left_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderCopyEx(renderer, render_texture, nullptr, &body, 0.0, nullptr, flip);
    } else {
        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderFillRect(renderer, &body);
    }

    if (punch_timer_ > 0.0f) {
        SDL_Rect fist = facing_left_
            ? SDL_Rect{body.x - 20, body.y + 24, 20, 12}
            : SDL_Rect{body.x + body.w, body.y + 24, 20, 12};
        SDL_SetRenderDrawColor(renderer, 255, 140, 90, 255);
        SDL_RenderFillRect(renderer, &fist);
    }
}
