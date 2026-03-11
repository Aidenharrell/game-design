#pragma once
#include <SDL.h>
#include <vector>
#include "input.hpp"
#include "player.hpp"
#include "platform.hpp"
#include "texture_set.hpp"

class Game {
public:
    bool Init();
    void Run();
    void Shutdown();

private:
    void HandleEvents();
    void Update(float dt);
    void Render();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    TextureSet player_texture_{};
    TextureSet idle_textures_{};
    TextureSet walk_textures_{};
    TextureSet punch_textures_{};
    TextureSet jump_textures_{};
    TextureSet heel_kick_textures_{};
    std::vector<Platform> platforms_;
    

    bool running_ = false;

    float camera_x_ = 0.0f;

    InputState input_{};
    Player player_{};
};
