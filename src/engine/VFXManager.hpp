#ifndef VFXMANAGER_HPP
#define VFXMANAGER_HPP

#include <vector>
#include <algorithm>
#include <SDL2/SDL.h>
#include "../core/Vec2.hpp"
#include "../core/Enums.hpp"
#include "../core/Constants.hpp"

class VFXManager {
public:
    float flashAlpha = 0.0f;
    std::vector<Particle> particles;

    void triggerFlash(float a) { flashAlpha = a; }
    void update(float dt) {
        if (flashAlpha > 0) flashAlpha -= 2.0f * dt;
        for (auto& p : particles) { p.pos = p.pos + p.vel * dt; p.life -= dt; }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0; }), particles.end());
    }
    void spawnBurst(Vec2 p, int n, SDL_Color c) {
        for (int i = 0; i < n; ++i) {
            float a = (float)(rand() % 360) * 0.0174f;
            float s = 40.0f + (rand() % 120);
            particles.push_back({p, {std::cos(a) * s, std::sin(a) * s}, 0.4f, 0.4f, c, 2.0f + (rand() % 2)});
        }
    }
    void render(SDL_Renderer* ren, const Vec2& cam) {
        for (const auto& p : particles) {
            SDL_SetRenderDrawColor(ren, p.color.r, p.color.g, p.color.b, (Uint8)(255 * (p.life / p.maxLife)));
            SDL_Rect r = {(int)(p.pos.x - cam.x), (int)(p.pos.y - cam.y), (int)p.size, (int)p.size};
            SDL_RenderFillRect(ren, &r);
        }
        if (flashAlpha > 0) {
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(flashAlpha * 255));
            SDL_Rect r = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_RenderFillRect(ren, &r);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        }
    }
};

#endif