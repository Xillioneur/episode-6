#ifndef SLUG_HPP
#define SLUG_HPP

#include "../engine/Entity.hpp"
#include <vector>

class KineticSlug : public Entity {
public:
    int bounces = 4;
    float powerMultiplier = 1.0f;
    bool isPlayer;
    std::vector<Vec2> tail;
    AmmoType ammoType = AmmoType::STANDARD;

    KineticSlug(Vec2 p, Vec2 v, bool pOwned, AmmoType at = AmmoType::STANDARD);
    void update(float dt, const std::vector<std::vector<Tile>>& map) override;
    bool checkWall(const std::vector<std::vector<Tile>>& map);
    void handleBounce();
    void render(SDL_Renderer* ren, const Vec2& camera) override;
};

#endif
