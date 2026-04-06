#pragma once
#include <SDL.h>
#include <vector>
#include "input.hpp"
#include "player.hpp"
#include "platform.hpp"
<<<<<<< HEAD
#include "enemy.hpp"
=======
>>>>>>> 49b8015 ( takes off tex_h and tex_w uses texture set now)
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
<<<<<<< HEAD
    TextureSet platform_textures_{};
    TextureSet ground_texture_{};
    TextureSet background_texture_{};
    TextureSet tree_texture_{};
    TextureSet bush_texture_{};
=======
>>>>>>> 49b8015 ( takes off tex_h and tex_w uses texture set now)
    std::vector<Platform> platforms_;
    std::vector<SquirrelEnemy> squirrels_;
    

    bool running_ = false;

    float camera_x_ = 0.0f;

    InputState input_{};
    Player player_{};
};
