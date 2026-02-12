#include "Slug.hpp"
#include "../core/Constants.hpp"

KineticSlug::KineticSlug(Vec2 p, Vec2 v, bool pOwned, AmmoType at)
    : Entity(p, 6, 6, EntityType::KINETIC_SLUG), isPlayer(pOwned), ammoType(at) {
    vel = v;
}

void KineticSlug::update(float dt, const std::vector<std::vector<Tile>>& map) {
    tail.push_back(pos);
    if (tail.size() > 12) tail.erase(tail.begin());

    Vec2 delta = vel * dt;
    float dist = delta.length();
    int steps = (int)(dist / 4.0f) + 1;
    Vec2 step = delta / (float)steps;

    for (int i = 0; i < steps; ++i) {
        pos.x += step.x;
        if (checkWall(map)) {
            pos.x -= step.x;
            vel.x *= -1;
            handleBounce();
        }
        pos.y += step.y;
        if (checkWall(map)) {
            pos.y -= step.y;
            vel.y *= -1;
            handleBounce();
        }
    }
    bounds.x = pos.x;
    bounds.y = pos.y;
    if (pos.x < 0 || pos.y < 0 || pos.x > MAP_WIDTH * TILE_SIZE || pos.y > MAP_HEIGHT * TILE_SIZE) active = false;
}

bool KineticSlug::checkWall(const std::vector<std::vector<Tile>>& map) {
    int bx = (int)(pos.x / TILE_SIZE);
    int by = (int)(pos.y / TILE_SIZE);
    return (bx >= 0 && bx < MAP_WIDTH && by >= 0 && by < MAP_HEIGHT && map[by][bx].type == WALL);
}

void KineticSlug::handleBounce() {
    bounces--;
    powerMultiplier += 0.65f;
    if (bounces < 0) active = false;
}

void KineticSlug::render(SDL_Renderer* ren, const Vec2& camera) {
    if (!active) return;
    SDL_Color trailCol = isPlayer ? (ammoType == AmmoType::EMP ? COL_EMP : (ammoType == AmmoType::PIERCING ? COL_GOLD : COL_PLAYER)) : COL_ROGUE_SLUG;
    for (size_t i = 0; i < tail.size(); ++i) {
        SDL_SetRenderDrawColor(ren, trailCol.r, trailCol.g, trailCol.b, (Uint8)(60 * (i / (float)tail.size())));
        SDL_Rect tr = {(int)(tail[i].x - camera.x), (int)(tail[i].y - camera.y), 4, 4};
        SDL_RenderFillRect(ren, &tr);
    }
    if (isPlayer) SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    else SDL_SetRenderDrawColor(ren, COL_ROGUE_SLUG.r, COL_ROGUE_SLUG.g, COL_ROGUE_SLUG.b, 255);
    SDL_Rect r = {(int)(pos.x - camera.x), (int)(pos.y - camera.y), (int)bounds.w, (int)bounds.h};
    SDL_RenderFillRect(ren, &r);
}