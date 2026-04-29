#pragma once
#include <SDL.h>

struct InputState {
    bool move_left = false;
    bool move_right = false;
    bool move_up = false;
    bool move_down = false;
    bool jump_pressed = false;
    bool punch_pressed = false;
    bool heel_kick_pressed = false;

    void ClearFrame() {
        jump_pressed = false;
        punch_pressed = false;
        heel_kick_pressed = false;
    }

    void OnKeyDown(SDL_Keycode key) {
        if (key == SDLK_a || key == SDLK_LEFT) move_left = true;
        if (key == SDLK_d || key == SDLK_RIGHT) move_right = true;
        if (key == SDLK_w || key == SDLK_UP) move_up = true;
        if (key == SDLK_s || key == SDLK_DOWN) move_down = true;
        if (key == SDLK_SPACE) jump_pressed = true;
        if (key == SDLK_j) punch_pressed = true;
        if (key == SDLK_k) heel_kick_pressed = true;
    }

    void OnKeyUp(SDL_Keycode key) {
        if (key == SDLK_a || key == SDLK_LEFT) move_left = false;
        if (key == SDLK_d || key == SDLK_RIGHT) move_right = false;
        if (key == SDLK_w || key == SDLK_UP) move_up = false;
        if (key == SDLK_s || key == SDLK_DOWN) move_down = false;
    }
};