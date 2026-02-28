#pragma once
#include <SDL.h>
#include <vector>
#include "input.hpp"

class Player {
public:
    void SetPosition(float x, float y) { x_ = x; y_ = y; }
    void SetGroundY(float y) { ground_y_ = y; }
    void SetTexture(SDL_Texture* texture, int w, int h);
    void SetIdleTextures(const std::vector<SDL_Texture*>& textures, int w, int h);
    void SetWalkTextures(const std::vector<SDL_Texture*>& textures, int w, int h);
    void SetPunchTextures(const std::vector<SDL_Texture*>& textures, int w, int h);

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
    float punch_frame_time_ = 0.0f;
    int punch_frame_ = 0;

    SDL_Texture* texture_ = nullptr;
    int base_tex_w_ = 0;
    int base_tex_h_ = 0;

    std::vector<SDL_Texture*> idle_textures_{};
    int idle_tex_w_ = 0;
    int idle_tex_h_ = 0;
    int idle_frame_ = 0;
    float idle_frame_time_ = 0.0f;
    bool idle_active_ = false;

    std::vector<SDL_Texture*> walk_textures_{};
    int walk_tex_w_ = 0;
    int walk_tex_h_ = 0;
    int walk_frame_ = 0;
    float walk_frame_time_ = 0.0f;
    bool walk_active_ = false;

    std::vector<SDL_Texture*> punch_textures_{};
    int punch_tex_w_ = 0;
    int punch_tex_h_ = 0;

    bool facing_left_ = false;
};
