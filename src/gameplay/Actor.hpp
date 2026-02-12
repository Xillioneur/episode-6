#ifndef ACTOR_HPP
#define ACTOR_HPP

#include "../engine/Entity.hpp"
#include <string>

// Forward decl
namespace Graphics {
    void drawWeapon(SDL_Renderer* ren, Vec2 center, float lookAngle, int length, int width, SDL_Color col, float handOffset = 0.0f);
    void drawContainment(SDL_Renderer* ren, SDL_Rect r);
}

class Player : public Entity {
public:
    float suitIntegrity = 100.0f;
    float energy = 100.0f;
    float reflexMeter = 100.0f;
    int slugs = 12;
    int maxSlugs = 12;
    int reserveSlugs = 60;
    float dashTimer = 0.0f;
    bool reflexActive = false;
    float shootCooldown = 0.0f;

    Player(Vec2 p);
    void update(float dt, const std::vector<std::vector<Tile>>& map) override;
    void render(SDL_Renderer* ren, const Vec2& cam) override;
};

class RogueCore : public Entity {
public:
    float stability = 100.0f;
    bool contained = false;
    bool sanitized = false;
    float stateTimer = 0.0f;
    float stunTimer = 0.0f;
    std::vector<Vec2> path;
    size_t pathIndex = 0;

    RogueCore(Vec2 p);
    RogueCore(Vec2 p, float w, float h, EntityType t);
    void render(SDL_Renderer* ren, const Vec2& cam) override;
    void calculatePath(const Vec2& target, const std::vector<std::vector<Tile>>& map);
};

class GuardianCore : public RogueCore {
public:
    float shield = 200.0f;
    GuardianCore(Vec2 p);
    void render(SDL_Renderer* ren, const Vec2& cam) override;
};

class SeekerSwarm : public RogueCore {
public:
    float angleOffset;
    SeekerSwarm(Vec2 p);
    void update(float dt, const std::vector<std::vector<Tile>>& map) override;
    void render(SDL_Renderer* ren, const Vec2& cam) override;
};

class RepairDrone : public RogueCore {
public:
    float repairPower = 15.0f;
    RogueCore* target = nullptr;
    RepairDrone(Vec2 p);
    void render(SDL_Renderer* ren, const Vec2& cam) override;
};

class FinalBossCore : public RogueCore {
public:
    int phase = 1;
    float phaseTimer = 0.0f;
    FinalBossCore(Vec2 p);
    void render(SDL_Renderer* ren, const Vec2& cam) override;
};

#endif
