#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <vector>
#include <SDL2/SDL.h>
#include "../core/Vec2.hpp"
#include "../core/Rect.hpp"
#include "../core/Enums.hpp"

class Entity {
public:
    Vec2 pos;
    Vec2 vel;
    Rect bounds;
    EntityType type;
    bool active = true;
    float lookAngle = 0.0f;

    Entity(Vec2 p, float w, float h, EntityType t);
    virtual ~Entity() {}

    virtual void update(float dt, const std::vector<std::vector<Tile>>& map);
    void move(Vec2 delta, const std::vector<std::vector<Tile>>& map);
    void collideMap(const std::vector<std::vector<Tile>>& map, bool xAxis, float moveDir);
    virtual void render(SDL_Renderer* ren, const Vec2& cam);
};

#endif