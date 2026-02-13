#ifndef ENVIRONMENTAL_HPP
#define ENVIRONMENTAL_HPP

#include "../engine/Entity.hpp"
#include "../engine/AudioManager.hpp"
#include "Actor.hpp"

class DecorativeMachine : public Entity {
public:
    SoundType sound;
    float timer = 0.0f;
    SDL_Color color;

    DecorativeMachine(Vec2 p, SoundType s, SDL_Color c) : Entity(p, 32, 32, EntityType::DECORATION), sound(s), color(c) {
        timer = (float)(rand() % 500) / 100.0f;
    }

    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
        (void)map;
        timer -= dt;
        if (timer <= 0) {
            timer = 3.0f + (float)(rand() % 400) / 100.0f;
            // Sound is played by Game
        }
    }

    void render(SDL_Renderer* ren, const Vec2& cam) override {
        SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        SDL_SetRenderDrawColor(ren, 40, 40, 50, 255); SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, 255);
        float pulse = std::abs(std::sin(SDL_GetTicks() * 0.002f));
        SDL_Rect light = {r.x + 10, r.y + 10, 12, 12};
        SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, (Uint8)(100 + pulse * 155));
        SDL_RenderFillRect(ren, &light);
        SDL_RenderDrawRect(ren, &r);
    }
};

#endif
