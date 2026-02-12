#ifndef INPUTHANDLER_HPP
#define INPUTHANDLER_HPP

#include <SDL2/SDL.h>
#include <algorithm>
#include <iterator>
#include "../core/Vec2.hpp"

class InputHandler {
public:
    bool keys[SDL_NUM_SCANCODES] = {false};
    bool lastKeys[SDL_NUM_SCANCODES] = {false};
    bool mDown = false;
    Vec2 mPos;

    void update() {
        std::copy(std::begin(keys), std::end(keys), std::begin(lastKeys));
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) exit(0);
            if (e.type == SDL_KEYDOWN) keys[e.key.keysym.scancode] = true;
            if (e.type == SDL_KEYUP) keys[e.key.keysym.scancode] = false;
            if (e.type == SDL_MOUSEBUTTONDOWN) mDown = true;
            if (e.type == SDL_MOUSEBUTTONUP) mDown = false;
            if (e.type == SDL_MOUSEMOTION) {
                mPos.x = (float)e.motion.x;
                mPos.y = (float)e.motion.y;
            }
        }
    }
    bool isPressed(SDL_Scancode c) const { return keys[c]; }
    bool isTriggered(SDL_Scancode c) const { return keys[c] && !lastKeys[c]; }
};

#endif
