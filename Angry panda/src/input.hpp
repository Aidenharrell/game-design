#pragma once
#include <SDL.h>

struct InputState {
    bool move_left = false;
    bool move_right = false;
    bool jump_pressed = false;
    bool punch_pressed = false;

    void ClearFrame() {
        jump_pressed = false;
        punch_pressed = false;
    }

    void OnKeyDown(SDL_Keycode key) {
        if (key == SDLK_a || key == SDLK_LEFT) move_left = true;
        if (key == SDLK_d || key == SDLK_RIGHT) move_right = true;
        if (key == SDLK_SPACE) jump_pressed = true;
        if (key == SDLK_j) punch_pressed = true;
    }

    void OnKeyUp(SDL_Keycode key) {
        if (key == SDLK_a || key == SDLK_LEFT) move_left = false;
        if (key == SDLK_d || key == SDLK_RIGHT) move_right = false;
    }
};
