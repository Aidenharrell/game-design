#pragma once
#include <SDL.h>
#include "input.hpp"

class Player {
public:
    void SetPosition(float x, float y) { x_ = x; y_ = y; }
    void SetGroundY(float y) { ground_y_ = y; }
    void SetTexture(SDL_Texture* texture, int w, int h) { texture_ = texture; tex_w_ = w; tex_h_ = h; }

    void Update(float dt, const InputState& input);
    void Render(SDL_Renderer* renderer) const;

private:
    float x_ = 0.0f;
    float y_ = 0.0f;
    float vx_ = 0.0f;
    float vy_ = 0.0f;

    float ground_y_ = 0.0f;
    bool on_ground_ = false;

    float punch_timer_ = 0.0f;

    SDL_Texture* texture_ = nullptr;
    int tex_w_ = 0;
    int tex_h_ = 0;
};
