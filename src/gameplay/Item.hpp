#ifndef ITEM_HPP
#define ITEM_HPP

#include "../engine/Entity.hpp"

class Item : public Entity {
public:
    ItemType it;
    Item(Vec2 p, ItemType _it) : Entity(p, 20, 20, EntityType::ITEM), it(_it) {
        vel = {0, 0};
    }

    void render(SDL_Renderer* r, const Vec2& cam) override {
        if (!active) return;
        SDL_Rect dr = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        if (it == ItemType::REPAIR_KIT) SDL_SetRenderDrawColor(r, 0, 255, 100, 255);
        else if (it == ItemType::BATTERY_PACK) SDL_SetRenderDrawColor(r, 255, 255, 0, 255);
        else if (it == ItemType::COOLANT) SDL_SetRenderDrawColor(r, 0, 150, 255, 255);
        else SDL_SetRenderDrawColor(r, 255, 100, 0, 255);
        SDL_RenderFillRect(r, &dr);
        SDL_SetRenderDrawColor(r, 255, 255, 255, 150);
        SDL_RenderDrawRect(r, &dr);
        SDL_Rect dot = {dr.x + 8, dr.y + 8, 4, 4};
        SDL_RenderFillRect(r, &dot);
    }
};

class NeuralEcho : public Entity {
public:
    float life = 4.0f;
    NeuralEcho(Vec2 p) : Entity(p, 32, 32, EntityType::NEURAL_ECHO) {}

    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
        (void)map;
        life -= dt;
        if (life <= 0) active = false;
    }

    void render(SDL_Renderer* ren, const Vec2& cam) override {
        if (!active) return;
        SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, COL_GLITCH.r, COL_GLITCH.g, COL_GLITCH.b, (Uint8)(100 + std::sin(SDL_GetTicks() * 0.01f) * 50));
        SDL_RenderFillRect(ren, &r);
        for(int i=0; i<4; ++i) {
            SDL_Rect frag = {r.x + (rand()%r.w), r.y + (rand()%r.h), 4, 2};
            SDL_RenderFillRect(ren, &frag);
        }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }
};

#endif
