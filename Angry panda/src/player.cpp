#include "player.hpp"
#include <algorithm>

static const float kMoveSpeed = 260.0f;
static const float kJumpVelocity = -520.0f;
static const float kGravity = 1400.0f;
static const float kPunchDuration = 0.18f;

void Player::Update(float dt, const InputState& input) {
    float move = 0.0f;
    if (input.move_left) move -= 1.0f;
    if (input.move_right) move += 1.0f;
    vx_ = move * kMoveSpeed;

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
}

void Player::Render(SDL_Renderer* renderer) const {
    SDL_Rect body{static_cast<int>(x_), static_cast<int>(y_) - 64, 48, 64};
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
    SDL_RenderFillRect(renderer, &body);

    if (punch_timer_ > 0.0f) {
        SDL_Rect fist{body.x + body.w, body.y + 24, 20, 12};
        SDL_SetRenderDrawColor(renderer, 255, 140, 90, 255);
        SDL_RenderFillRect(renderer, &fist);
    }
}
