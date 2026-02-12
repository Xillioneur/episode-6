/*
 * Recoil Protocol: Graphics Overdrive - Final Edition
 * ----------------------------------------------------------------------------
 * A high-polish Top-Down Tactical Action Game.
 * Theme: Cybernetic Security Inspector
 *
 * MISSION:
 * Neutralize Rogue AI Cores using Kinetic Impactors.
 * Bounce slugs off walls to increase energy and lethality.
 *
 * GRAPHICS ENGINE:
 * - Procedural Multi-Part Models: Every entity is composed of several geometry layers.
 * - Centered Weapon Systems: Realistic tracking weapons centered on entity axis.
 * - Advanced Containment: Unified pulsing grid field for all vulnerable cores.
 * - Environment Greebles: Circuitry patterns, floor decals, and wall pipes.
 *
 * Compile:
 * g++ main.cpp -o shadowrecon -std=c++17 -lSDL2 -lSDL2_ttf
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <random>
#include <ctime>
#include <queue>
#include <map>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// ============================================================================
// [GLOBAL CONSTANTS]
// ============================================================================

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE = 40;
const int MAP_WIDTH = 50;
const int MAP_HEIGHT = 50;
const int TARGET_FPS = 60;
const float FRAME_DELAY = 1000.0f / TARGET_FPS;

const float PLAYER_SPEED = 220.0f;
const float DASH_SPEED = 850.0f;
const float AI_SPEED = 140.0f;
const float REFLEX_SCALE = 0.25f;

// Neon Cyberpunk Palette
const SDL_Color COL_BG = {5, 5, 10, 255};
const SDL_Color COL_WALL = {35, 40, 55, 255};
const SDL_Color COL_FLOOR = {15, 15, 20, 255};
const SDL_Color COL_PLAYER = {50, 255, 150, 255}; 
const SDL_Color COL_CORE = {255, 80, 50, 255};    
const SDL_Color COL_SLUG = {255, 255, 255, 255};
const SDL_Color COL_ROGUE_SLUG = {255, 150, 50, 255};
const SDL_Color COL_GLITCH = {200, 50, 255, 180};
const SDL_Color COL_TEXT = {220, 230, 255, 255};
const SDL_Color COL_GOLD = {255, 215, 0, 255};
const SDL_Color COL_EMP = {100, 150, 255, 255};
const SDL_Color COL_CONTAINED = {100, 200, 255, 255};

// ============================================================================
// [MATH & UTILITIES]
// ============================================================================

struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
    Vec2 operator+(const Vec2& v) const { return {x + v.x, y + v.y}; }
    Vec2 operator-(const Vec2& v) const { return {x - v.x, y - v.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const {
        float l = length();
        return (l > 0.0001f) ? Vec2(x / l, y / l) : Vec2(0, 0);
    }
    float distance(const Vec2& v) const { return (*this - v).length(); }
};

struct Rect {
    float x, y, w, h;
    bool intersects(const Rect& other) const {
        return (x < other.x + other.w && x + w > other.x &&
                y < other.y + other.h && y + h > other.y);
    }
    bool contains(const Vec2& p) const {
        return (p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h);
    }
    Vec2 center() const { return {x + w / 2, y + h / 2}; }
};

enum class GameState { MENU, PLAYING, GAME_OVER, VICTORY };
enum class EntityType { PLAYER, ROGUE_CORE, NEURAL_ECHO, KINETIC_SLUG, ITEM, EXIT, HAZARD, DECORATION, GADGET };
enum class AmmoType { STANDARD, EMP, PIERCING };
enum class ItemType { REPAIR_KIT, BATTERY_PACK, COOLANT, OVERCLOCK };
enum TileType { WALL, FLOOR };

struct Tile {
    TileType type;
    Rect rect;
};

struct Particle {
    Vec2 pos; Vec2 vel; float life; float maxLife; SDL_Color color; float size;
};

struct FloatingText {
    Vec2 pos; std::string text; float life; SDL_Color color;
};

struct SaveData {
    int sector; int score; float integrity;
};

void saveProgress(const SaveData& data) {
    FILE* f = fopen("recoil_save.bin", "wb");
    if (f) {
        fwrite(&data, sizeof(SaveData), 1, f);
        fclose(f);
    }
}

// ============================================================================
// [GRAPHICS ENGINE]
// ============================================================================

namespace Graphics {
    void drawCircle(SDL_Renderer* ren, int cx, int cy, int r) {
        for (double dy = 1; dy <= r; dy += 1.0) {
            double dx = floor(std::sqrt((2.0 * r * dy) - (dy * dy)));
            SDL_RenderDrawLine(ren, cx - (int)dx, cy + (int)r - (int)dy, cx + (int)dx, cy + (int)r - (int)dy);
            SDL_RenderDrawLine(ren, cx - (int)dx, cy - (int)r + (int)dy, cx + (int)dx, cy - (int)r + (int)dy);
        }
    }

    void drawGlowRect(SDL_Renderer* ren, SDL_Rect r, SDL_Color col) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, 50);
        SDL_Rect glow = {r.x - 4, r.y - 4, r.w + 8, r.h + 8};
        SDL_RenderDrawRect(ren, &glow);
        SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, 255);
        SDL_RenderDrawRect(ren, &r);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }

    void drawContainment(SDL_Renderer* ren, SDL_Rect r) {
        float pulse = std::abs(std::sin(SDL_GetTicks() * 0.01f));
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 100, 200, 255, (Uint8)(100 + pulse * 100));
        SDL_RenderDrawRect(ren, &r);
        SDL_Rect inner = {r.x + 4, r.y + 4, r.w - 8, r.h - 8};
        SDL_RenderDrawRect(ren, &inner);
        for(int i=0; i<r.w; i+=8) {
            SDL_RenderDrawLine(ren, r.x+i, r.y, r.x+i, r.y+r.h);
        }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }

    void drawWeapon(SDL_Renderer* ren, Vec2 center, float lookAngle, int length, int width, SDL_Color col, float handOffset = 0.0f) {
        float handAngle = lookAngle + 1.5708f;
        Vec2 handPos = center + Vec2(std::cos(handAngle) * handOffset, std::sin(handAngle) * handOffset);
        
        float c = std::cos(lookAngle);
        float s = std::sin(lookAngle);
        int ex = (int)(handPos.x + c * length);
        int ey = (int)(handPos.y + s * length);
        
        SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, 255);
        for(int i = -width/2; i <= width/2; ++i) {
            float ox = -s * i;
            float oy = c * i;
            SDL_RenderDrawLine(ren, (int)(handPos.x + ox), (int)(handPos.y + oy), (int)(ex + ox), (int)(ey + oy));
        }
        // Laser Sight
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 255, 50, 50, 80);
        SDL_RenderDrawLine(ren, (int)handPos.x, (int)handPos.y, (int)(handPos.x + c * 150.0f), (int)(handPos.y + s * 150.0f));
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }
}

// ============================================================================
// [ENTITY CORE SYSTEM]
// ============================================================================

class Entity {
public:
    Vec2 pos;
    Vec2 vel;
    Rect bounds;
    EntityType type;
    bool active = true;
    float lookAngle = 0.0f;

    Entity(Vec2 p, float w, float h, EntityType t) : pos(p), type(t) {
        bounds = {p.x, p.y, w, h};
    }
    virtual ~Entity() {}

    virtual void update(float dt, const std::vector<std::vector<Tile>>& map) {
        move(vel * dt, map);
    }

    void move(Vec2 delta, const std::vector<std::vector<Tile>>& map) {
        float dist = delta.length();
        if (dist <= 0) {
            bounds.x = pos.x;
            bounds.y = pos.y;
            return;
        }

        int steps = (int)(dist / 4.0f) + 1;
        Vec2 step = delta / (float)steps;

        for (int i = 0; i < steps; ++i) {
            if (std::abs(step.x) > 0.0001f) {
                pos.x += step.x;
                collideMap(map, true, step.x);
            }
            if (std::abs(step.y) > 0.0001f) {
                pos.y += step.y;
                collideMap(map, false, step.y);
            }
        }

        pos.x = std::clamp(pos.x, 40.0f, (MAP_WIDTH - 2) * 40.0f - bounds.w);
        pos.y = std::clamp(pos.y, 40.0f, (MAP_HEIGHT - 2) * 40.0f - bounds.h);
        bounds.x = pos.x;
        bounds.y = pos.y;
    }

    void collideMap(const std::vector<std::vector<Tile>>& map, bool xAxis, float moveDir) {
        bounds.x = pos.x;
        bounds.y = pos.y;
        int minX = std::max(0, (int)(pos.x / TILE_SIZE));
        int maxX = std::min(MAP_WIDTH - 1, (int)((pos.x + bounds.w) / TILE_SIZE));
        int minY = std::max(0, (int)(pos.y / TILE_SIZE));
        int maxY = std::min(MAP_HEIGHT - 1, (int)((pos.y + bounds.h) / TILE_SIZE));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (map[y][x].type == WALL && bounds.intersects(map[y][x].rect)) {
                    if (xAxis) {
                        pos.x = (moveDir > 0) ? map[y][x].rect.x - bounds.w - 0.001f : map[y][x].rect.x + map[y][x].rect.w + 0.001f;
                        vel.x *= -0.2f; 
                    } else {
                        pos.y = (moveDir > 0) ? map[y][x].rect.y - bounds.h - 0.001f : map[y][x].rect.y + map[y][x].rect.h + 0.001f;
                        vel.y *= -0.2f;
                    }
                    bounds.x = pos.x;
                    bounds.y = pos.y;
                }
            }
        }
    }

    virtual void render(SDL_Renderer* ren, const Vec2& cam) {
        if (!active) return;
        SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
        SDL_RenderFillRect(ren, &r);
    }
};

// ============================================================================
// [ACTOR MODELS]
// ============================================================================

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

    Player(Vec2 p) : Entity(p, 24, 24, EntityType::PLAYER) {}

    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
        if (dashTimer > 0) dashTimer -= dt;
        Entity::update(dt, map);
        if (shootCooldown > 0) shootCooldown -= dt;
        energy = std::min(100.0f, energy + 12.0f * dt);
        if (reflexActive) {
            reflexMeter -= 25.0f * dt;
            if (reflexMeter <= 0) { reflexMeter = 0; reflexActive = false; }
        } else {
            reflexMeter = std::min(100.0f, reflexMeter + 15.0f * dt);
        }
    }

    void render(SDL_Renderer* ren, const Vec2& cam) override {
        SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        // Suit Base
        SDL_SetRenderDrawColor(ren, 40, 45, 55, 255);
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren, 20, 20, 25, 255);
        SDL_RenderDrawRect(ren, &r);
        
        // Armor Details
        SDL_Rect chest = {r.x + 4, r.y + 4, 16, 16};
        SDL_SetRenderDrawColor(ren, 60, 70, 80, 255);
        SDL_RenderFillRect(ren, &chest);
        
        // Cyan Visor
        SDL_Rect visor = {r.x + 6, r.y + 2, 12, 4};
        SDL_SetRenderDrawColor(ren, 100, 255, 255, 255);
        SDL_RenderFillRect(ren, &visor);

        // Centered Kinetic Rifle
        Graphics::drawWeapon(ren, { (float)r.x + 12, (float)r.y + 12 }, lookAngle, 22, 6, {100, 110, 120, 255}, 0.0f);

        if(dashTimer > 0) {
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 150);
            SDL_RenderDrawRect(ren, &r);
        }
    }
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

    RogueCore(Vec2 p) : Entity(p, 28, 28, EntityType::ROGUE_CORE) {
        vel = {0, 0};
    }

    void render(SDL_Renderer* ren, const Vec2& cam) override {
        if (!active) return;
        SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        
        if (sanitized) {
            SDL_SetRenderDrawColor(ren, 40, 45, 55, 255);
            SDL_RenderFillRect(ren, &r);
            SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
            SDL_RenderDrawRect(ren, &r);
            return;
        }

        // Core Housing
        SDL_SetRenderDrawColor(ren, 30, 30, 40, 255);
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren, COL_CORE.r, COL_CORE.g, COL_CORE.b, 255);
        SDL_RenderDrawRect(ren, &r);
        
        // Inner Logic Gate
        SDL_Rect core = {r.x + 8, r.y + 8, 12, 12};
        SDL_RenderFillRect(ren, &core);

        // Centered Security Cannon
        Graphics::drawWeapon(ren, { (float)r.x + 14, (float)r.y + 14 }, lookAngle, 18, 5, {80, 40, 40, 255}, 0.0f);

        if (contained) {
            Graphics::drawContainment(ren, r);
        }
    }

    void calculatePath(const Vec2& target, const std::vector<std::vector<Tile>>& map);
};

class GuardianCore : public RogueCore {
public:
    float shield = 200.0f;
    GuardianCore(Vec2 p) : RogueCore(p) {
        bounds.w = 52;
        bounds.h = 52;
        stability = 500.0f;
    }

    void render(SDL_Renderer* ren, const Vec2& cam) override {
        if (!active) return;
        if (sanitized) { RogueCore::render(ren, cam); return; }
        SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        
        // Reinforced Plating
        SDL_SetRenderDrawColor(ren, 70, 75, 90, 255);
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren, 120, 130, 150, 255);
        for(int i=0; i<3; ++i) {
            SDL_Rect plate = {r.x + i*4, r.y + i*4, r.w - i*8, r.h - i*8};
            SDL_RenderDrawRect(ren, &plate);
        }
        
        // Shield Emitter Node
        SDL_Rect emitter = {r.x + 20, r.y + 20, 12, 12};
        SDL_SetRenderDrawColor(ren, 100, 200, 255, 255);
        SDL_RenderFillRect(ren, &emitter);

        // Centered Heavy Cannon
        Graphics::drawWeapon(ren, { (float)r.x + 26, (float)r.y + 26 }, lookAngle, 26, 8, {100, 100, 120, 255}, 0.0f);

        if (shield > 0) {
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 100, 200, 255, 80);
            SDL_Rect sr = {r.x - 8, r.y - 8, r.w + 16, r.h + 16};
            SDL_RenderDrawRect(ren, &sr);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        }

        if (contained) Graphics::drawContainment(ren, r);
    }
};

class SeekerSwarm : public RogueCore {
public:
    float angleOffset;
    SeekerSwarm(Vec2 p) : RogueCore(p) {
        bounds.w = 20;
        bounds.h = 20;
        stability = 30.0f;
        angleOffset = (float)(rand() % 360);
    }

    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
        if (stunTimer <= 0) {
            angleOffset += 5.0f * dt;
            Vec2 orbit = {std::cos(angleOffset) * 40.0f, std::sin(angleOffset) * 40.0f};
            move(orbit * dt, map);
        }
        RogueCore::update(dt, map);
    }

    void render(SDL_Renderer* ren, const Vec2& cam) override {
        if (!active || sanitized) return;
        SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        
        // Blade Swarm Model
        SDL_SetRenderDrawColor(ren, 255, 150, 0, 255);
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderDrawLine(ren, r.x, r.y, r.x+r.w, r.y+r.h);
        SDL_RenderDrawLine(ren, r.x+r.w, r.y, r.x, r.y+r.h);
        
        // Centered Sting Barrel
        Graphics::drawWeapon(ren, { (float)r.x + 10, (float)r.y + 10 }, lookAngle, 14, 3, {200, 100, 0, 255}, 0.0f);

        if (contained) Graphics::drawContainment(ren, r);
    }
};

class RepairDrone : public RogueCore {
public:
    float repairPower = 15.0f;
    RogueCore* target = nullptr;
    RepairDrone(Vec2 p) : RogueCore(p) {
        bounds.w = 24;
        bounds.h = 24;
    }

    void render(SDL_Renderer* ren, const Vec2& cam) override {
        if (!active || sanitized) return;
        SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        
        // Logistics Unit Model
        SDL_SetRenderDrawColor(ren, 40, 80, 40, 255);
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren, 100, 255, 100, 255);
        SDL_RenderDrawRect(ren, &r);
        
        // Centered Welding Array
        Graphics::drawWeapon(ren, { (float)r.x + 12, (float)r.y + 12 }, lookAngle, 16, 4, {0, 255, 100, 255}, 0.0f);

        if (contained) Graphics::drawContainment(ren, r);
    }
};

class FinalBossCore : public RogueCore {
public:
    int phase = 1;
    float phaseTimer = 0.0f;
    FinalBossCore(Vec2 p) : RogueCore(p) {
        bounds.w = 96;
        bounds.h = 96;
        stability = 2500.0f;
    }

    void render(SDL_Renderer* ren, const Vec2& cam) override {
        if (!active) return;
        if (sanitized) { RogueCore::render(ren, cam); return; }
        SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        
        // Mega Fortress Chassis
        SDL_SetRenderDrawColor(ren, 15, 15, 20, 255);
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren, 200, 0, 50, 255);
        for(int i=0; i<6; ++i) {
            SDL_Rect rim = {r.x + i*3, r.y + i*3, r.w - i*6, r.h - i*6};
            SDL_RenderDrawRect(ren, &rim);
        }
        
        // Pulsing Ocular
        float pulse = std::abs(std::sin(SDL_GetTicks() * 0.005f)) * 40.0f;
        SDL_Rect eye = {r.x + 36, r.y + 36, 24, 24};
        SDL_SetRenderDrawColor(ren, 255, 50, (Uint8)(255 - pulse), 255);
        SDL_RenderFillRect(ren, &eye);

        // Radial Weapon Mounts
        Graphics::drawWeapon(ren, { (float)r.x + 48, (float)r.y + 48 }, lookAngle, 48, 12, {150, 50, 50, 255}, 0.0f);
        Graphics::drawWeapon(ren, { (float)r.x + 48, (float)r.y + 48 }, lookAngle + 0.8f, 38, 8, {120, 40, 40, 255}, 0.0f);
        Graphics::drawWeapon(ren, { (float)r.x + 48, (float)r.y + 48 }, lookAngle - 0.8f, 38, 8, {120, 40, 40, 255}, 0.0f);

        if (phase == 2) {
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 255, 50, 255, 120);
            SDL_Rect sr = {r.x - 16, r.y - 16, r.w + 32, r.h + 32};
            SDL_RenderDrawRect(ren, &sr);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        }

        if (contained) Graphics::drawContainment(ren, r);
    }
};

// ============================================================================
// [PROJECTILE & ASSET CLASSES]
// ============================================================================

class KineticSlug : public Entity {
public:
    int bounces = 4;
    float powerMultiplier = 1.0f;
    bool isPlayer;
    std::vector<Vec2> tail;
    AmmoType ammoType = AmmoType::STANDARD;

    KineticSlug(Vec2 p, Vec2 v, bool pOwned, AmmoType at = AmmoType::STANDARD)
        : Entity(p, 6, 6, EntityType::KINETIC_SLUG), isPlayer(pOwned), ammoType(at) {
        vel = v;
    }

    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
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

    bool checkWall(const std::vector<std::vector<Tile>>& map) {
        int bx = (int)(pos.x / TILE_SIZE);
        int by = (int)(pos.y / TILE_SIZE);
        return (bx >= 0 && bx < MAP_WIDTH && by >= 0 && by < MAP_HEIGHT && map[by][bx].type == WALL);
    }

    void handleBounce() {
        bounces--;
        powerMultiplier += 0.65f;
        if (bounces < 0) active = false;
    }

    void render(SDL_Renderer* ren, const Vec2& camera) override {
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
};

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

// ============================================================================
// [SYSTEM MANAGERS]
// ============================================================================

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

class LightingManager {
public:
    std::vector<std::vector<float>> lMap;
    LightingManager() {
        lMap.resize(MAP_HEIGHT);
        for (auto& r : lMap) r.assign(MAP_WIDTH, 0.0f);
    }

    void update(const Vec2& cp, const std::vector<std::vector<Tile>>& map) {
        for (auto& r : lMap) std::fill(r.begin(), r.end(), 0.1f);
        for (int i = 0; i < 360; i += 2) {
            float a = (float)i * 0.0174f;
            float c = std::cos(a), s = std::sin(a), d = 0;
            while (d < 450.0f) {
                d += 15.0f;
                int tx = (int)((cp.x + c * d) / TILE_SIZE), ty = (int)((cp.y + s * d) / TILE_SIZE);
                if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT) {
                    float v = 1.0f - (d / 450.0f);
                    if (v > lMap[ty][tx]) lMap[ty][tx] = v;
                    if (map[ty][tx].type == WALL) break;
                } else break;
            }
        }
    }

    void render(SDL_Renderer* ren, const Vec2& cam) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        int sx = std::max(0, (int)(cam.x / TILE_SIZE));
        int sy = std::max(0, (int)(cam.y / TILE_SIZE));
        int ex = std::min(MAP_WIDTH, (int)((cam.x + SCREEN_WIDTH) / TILE_SIZE) + 1);
        int ey = std::min(MAP_HEIGHT, (int)((cam.y + SCREEN_HEIGHT) / TILE_SIZE) + 1);
        for (int y = sy; y < ey; ++y) {
            for (int x = sx; x < ex; ++x) {
                int a = (int)(255.0f * (1.0f - lMap[y][x]));
                if (a > 0) {
                    SDL_SetRenderDrawColor(ren, 0, 0, 5, (Uint8)a);
                    SDL_Rect r = {(int)(x * TILE_SIZE - cam.x), (int)(y * TILE_SIZE - cam.y), TILE_SIZE, TILE_SIZE};
                    SDL_RenderFillRect(ren, &r);
                }
            }
        }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }
};

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

// ============================================================================
// [GAME CLASS]
// ============================================================================

class Game;

class ObjectiveSystem {
public:
    enum Type { CLEAR_CORES, REACH_EXIT };
    Type currentType = CLEAR_CORES;

    void update(Game& game);
    std::string getDesc() const {
        return (currentType == CLEAR_CORES) ? "OBJECTIVE: Neutralize Rogue AI Cores." : "OBJECTIVE: Proceed to extraction point.";
    }
};

class HUD {
public:
    struct LogEntry { std::string msg; float life; SDL_Color col; };
    std::vector<LogEntry> logs;

    void addLog(const std::string& m, SDL_Color c = {200, 200, 255, 255}) {
        logs.push_back({m, 5.0f, c});
        if (logs.size() > 6) logs.erase(logs.begin());
    }

    void update(float dt) {
        for (auto& l : logs) l.life -= dt;
        logs.erase(std::remove_if(logs.begin(), logs.end(), [](const LogEntry& le) { return le.life <= 0; }), logs.end());
    }

    void render(SDL_Renderer* ren, Player* p, int score, int sector, Game& game, TTF_Font* font, TTF_Font* fontL);
    void renderText(SDL_Renderer* ren, const std::string& t, int x, int y, TTF_Font* f, SDL_Color c);

private:
    void drawBar(SDL_Renderer* ren, int x, int y, int w, int h, float pct, SDL_Color col) {
        SDL_Rect bg = {x, y, w, h};
        SDL_SetRenderDrawColor(ren, 40, 40, 40, 255);
        SDL_RenderFillRect(ren, &bg);
        SDL_Rect fg = {x, y, (int)(w * std::clamp(pct, 0.0f, 1.0f)), h};
        SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
        SDL_RenderFillRect(ren, &fg);
        SDL_SetRenderDrawColor(ren, 80, 80, 80, 255);
        SDL_RenderDrawRect(ren, &bg);
    }
};

class Game {
public:
    bool running = true;
    GameState state = GameState::MENU;
    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;
    TTF_Font *font = nullptr, *fontL = nullptr;

    std::vector<std::vector<Tile>> map;
    InputHandler input;
    LightingManager lighting;
    VFXManager vfx;
    ObjectiveSystem objective;
    HUD hud;

    Player* p = nullptr;
    std::vector<RogueCore*> cores;
    std::vector<KineticSlug*> slugs;
    std::vector<NeuralEcho*> echoes;
    std::vector<Item*> items;
    std::vector<FloatingText> fTexts;
    Entity* exit = nullptr;

    Vec2 cam = {0, 0};
    float shake = 0.0f;
    int score = 0;
    int sector = 1;
    bool debugMode = false;
    AmmoType currentAmmo = AmmoType::STANDARD;

    Game();
    ~Game();
    void init();
    void cleanup();
    void generateLevel();
    Vec2 findSpace(float w = 24, float h = 24);
    void handleInput();
    void update();
    void render();
    void renderT(std::string t, int x, int y, TTF_Font* f, SDL_Color c);

    void updateAI(float dt);
    void updateSlugs(float dt);
    void updateEchoes(float dt);
    void updatePickups();
    void updateWeapons(float dt);
    void spawnFText(Vec2 pos, std::string t, SDL_Color c);
    void loop();
};

// ============================================================================
// [IMPLEMENTATIONS]
// ============================================================================

void RogueCore::calculatePath(const Vec2& target, const std::vector<std::vector<Tile>>& map) {
    int sx=(int)(bounds.center().x/TILE_SIZE), sy=(int)(bounds.center().y/TILE_SIZE), ex=(int)(target.x/TILE_SIZE), ey=(int)(target.y/TILE_SIZE);
    if(sx==ex && sy==ey){path.clear(); return;}
    if(ex<0||ex>=MAP_WIDTH||ey<0||ey>=MAP_HEIGHT||map[ey][ex].type==WALL)return;
    static float gS[MAP_HEIGHT][MAP_WIDTH]; static std::pair<int,int> par[MAP_HEIGHT][MAP_WIDTH]; static bool vis[MAP_HEIGHT][MAP_WIDTH];
    for(int y=0; y<MAP_HEIGHT; ++y) for(int x=0; x<MAP_WIDTH; ++x) { gS[y][x]=1e6f; vis[y][x]=false; }
    std::priority_queue<std::pair<float, std::pair<int,int>>, std::vector<std::pair<float, std::pair<int,int>>>, std::greater<std::pair<float, std::pair<int,int>>>> pq;
    gS[sy][sx]=0; pq.push({0, {sx, sy}}); bool found=false;
    while(!pq.empty()){
        auto cur=pq.top().second; pq.pop(); int cx=cur.first, cy=cur.second; if(cx==ex && cy==ey){found=true; break;}
        if(vis[cy][cx])continue; vis[cy][cx]=true; int dx[]={0,0,1,-1}, dy[]={1,-1,0,0};
        for(int i=0; i<4; ++i){ int nx=cx+dx[i], ny=cy+dy[i]; if(nx>=0&&nx<MAP_WIDTH&&ny>=0&&ny<MAP_HEIGHT&&map[ny][nx].type!=WALL&&!vis[ny][nx]){ float tg=gS[cy][cx]+1.0f; if(tg<gS[ny][nx]){par[ny][nx]={cx,cy}; gS[ny][nx]=tg; pq.push({tg+(float)std::abs(nx-ex)+(float)std::abs(ny-ey), {nx,ny}}); } } }
    }
    path.clear(); if(found){ int cx=ex, cy=ey; while(cx!=sx||cy!=sy){ path.push_back({(float)cx*TILE_SIZE+20, (float)cy*TILE_SIZE+20}); auto p=par[cy][cx]; cx=p.first; cy=p.second; } std::reverse(path.begin(), path.end()); pathIndex=0; }
}

void HUD::renderText(SDL_Renderer* ren, const std::string& t, int x, int y, TTF_Font* f, SDL_Color c) {
    if (!f || t.empty()) return; SDL_Surface* s = TTF_RenderText_Blended(f, t.c_str(), c);
    if (s) { SDL_Texture* tx = SDL_CreateTextureFromSurface(ren, s); SDL_Rect dst = {x, y, s->w, s->h}; SDL_RenderCopy(ren, tx, NULL, &dst); SDL_DestroyTexture(tx); SDL_FreeSurface(s); }
}

void HUD::render(SDL_Renderer* ren, Player* p, int score, int sector, Game& game, TTF_Font* font, TTF_Font* fontL) {
    (void)fontL;
    drawBar(ren, 20, 20, 200, 18, p->suitIntegrity/100.0f, {50, 255, 100, 255}); renderText(ren, "INTEGRITY", 25, 21, font, {255, 255, 255, 255});
    drawBar(ren, 20, 45, 150, 10, p->energy/100.0f, {50, 150, 255, 255}); renderText(ren, "ENERGY", 25, 45, font, {200, 200, 255, 255});
    drawBar(ren, 20, 60, 150, 10, p->reflexMeter / 100.0f, {255, 200, 50, 255}); renderText(ren, "REFLEX", 25, 60, font, {255, 255, 200, 255});
    
    renderText(ren, "SLUGS: " + std::to_string(p->slugs) + " / " + std::to_string(p->reserveSlugs), 20, 85, font, {220, 220, 220, 255});
    std::string ammoStr = (game.currentAmmo == AmmoType::STANDARD) ? "STANDARD" : (game.currentAmmo == AmmoType::EMP) ? "EMP" : "PIERCING";
    renderText(ren, "AMMO: " + ammoStr, 20, 100, font, {150, 255, 255, 255});
    
    if(game.debugMode) renderText(ren, "DEBUG ACTIVE", SCREEN_WIDTH-150, 20, font, {255, 255, 0, 255});
    renderText(ren, "SECTOR: " + std::to_string(sector), SCREEN_WIDTH-120, 40, font, {150, 150, 255, 255});
    renderText(ren, "SCORE: " + std::to_string(score), SCREEN_WIDTH-120, 60, font, {255, 255, 255, 255});
    renderText(ren, game.objective.getDesc(), SCREEN_WIDTH/2-150, 20, font, {255, 255, 100, 255});
    
    SDL_Rect mmRect = {SCREEN_WIDTH - 110, 100, 100, 100}; SDL_SetRenderDrawColor(ren, 0, 0, 0, 180); SDL_RenderFillRect(ren, &mmRect);
    float mapScale = 0.04f; Vec2 mapCtr = {(float)mmRect.x + 50, (float)mmRect.y + 50};
    for(auto c : game.cores) { if(!c->sanitized) { Vec2 rel = (c->pos - p->pos) * mapScale; if(std::abs(rel.x)<48 && std::abs(rel.y)<48) { SDL_Rect d = {(int)(mapCtr.x+rel.x-1), (int)(mapCtr.y+rel.y-1), 3, 3}; SDL_SetRenderDrawColor(ren, 255, 50, 50, 255); SDL_RenderFillRect(ren, &d); } } }
    if(game.exit && game.exit->active) { Vec2 rel = (game.exit->pos - p->pos) * mapScale; if(std::abs(rel.x)<48 && std::abs(rel.y)<48) { SDL_Rect d = {(int)(mapCtr.x+rel.x-2), (int)(mapCtr.y+rel.y-2), 5, 5}; SDL_SetRenderDrawColor(ren, 100, 255, 100, 255); SDL_RenderFillRect(ren, &d); } }
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255); SDL_Rect pDot = {(int)mapCtr.x-2, (int)mapCtr.y-2, 4, 4}; SDL_RenderFillRect(ren, &pDot);

    if (game.exit && game.exit->active) {
        Vec2 dir = (game.exit->pos - p->pos).normalized();
        Vec2 arrowPos = { SCREEN_WIDTH / 2.0f + dir.x * 65.0f, SCREEN_HEIGHT / 2.0f + dir.y * 65.0f };
        SDL_Rect arrow = {(int)arrowPos.x - 4, (int)arrowPos.y - 4, 8, 8};
        SDL_SetRenderDrawColor(ren, 100, 255, 100, 255); SDL_RenderFillRect(ren, &arrow);
    }

    int logY = SCREEN_HEIGHT - 120; for (auto& l : logs) { renderText(ren, "> " + l.msg, 20, logY, font, l.col); logY += 18; }
}

void ObjectiveSystem::update(Game& game) {
    bool all = true; for (auto c : game.cores) if (!c->sanitized) { all = false; break; }
    if (all) { currentType = REACH_EXIT; if(game.exit) game.exit->active = true; } else currentType = CLEAR_CORES;
}

Game::Game() {
    SDL_Init(SDL_INIT_VIDEO); TTF_Init();
    win = SDL_CreateWindow("Recoil Protocol", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#ifdef __APPLE__
    font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 18); fontL = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 52);
#else
    font = TTF_OpenFont("arial.ttf", 18); fontL = TTF_OpenFont("arial.ttf", 52);
#endif
    init();
}
Game::~Game() { cleanup(); if(font)TTF_CloseFont(font); if(fontL)TTF_CloseFont(fontL); SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); TTF_Quit(); SDL_Quit(); }

void Game::init() {
    cleanup(); generateLevel(); p = new Player(findSpace()); p->reserveSlugs = 60;
    for(int i=0; i<5+sector*2; ++i) cores.push_back(new RogueCore(findSpace()));
    if(sector%2==0) for(int i=0; i<2+sector/2; ++i) cores.push_back(new SeekerSwarm(findSpace()));
    if(sector%5==0) { cores.push_back(new FinalBossCore(findSpace(80, 80))); hud.addLog("CRITICAL: BOSS ANOMALY DETECTED!", {255, 50, 50, 255}); }
    for(int i=0; i<8; ++i) items.push_back(new Item(findSpace(18, 18), (rand()%100<40)?ItemType::BATTERY_PACK:ItemType::REPAIR_KIT));
    exit = new Entity(findSpace(40, 40), 40, 40, EntityType::EXIT); exit->active = false; state = GameState::PLAYING;
    hud.addLog("SYSTEM ONLINE. SECTOR " + std::to_string(sector));
}

void Game::cleanup() {
    if(p) delete p; for(auto c:cores) delete c; for(auto s:slugs) delete s; for(auto e:echoes) delete e; for(auto i:items) delete i; if(exit) delete exit;
    cores.clear(); slugs.clear(); echoes.clear(); items.clear(); fTexts.clear();
}

Vec2 Game::findSpace(float w, float h) {
    for(int i=0; i<2000; ++i) {
        int x=1+rand()%(MAP_WIDTH-2), y=1+rand()%(MAP_HEIGHT-2);
        if(map[y][x].type==FLOOR) {
            Rect r = {(float)x*TILE_SIZE+2, (float)y*TILE_SIZE+2, w, h}; bool safe = true;
            for(int sy=y-1; sy<=y+2; sy++) for(int sx=x-1; sx<=x+2; sx++) {
                if(sx>=0 && sx<MAP_WIDTH && sy>=0 && sy<MAP_HEIGHT && map[sy][sx].type==WALL && r.intersects(map[sy][sx].rect)) safe = false;
            }
            if(safe) return {r.x, r.y};
        }
    }
    return {MAP_WIDTH*TILE_SIZE/2.0f, MAP_HEIGHT*TILE_SIZE/2.0f};
}

void Game::generateLevel() {
    bool connected = false;
    while(!connected) {
        map.assign(MAP_HEIGHT, std::vector<Tile>(MAP_WIDTH));
        for(int y=0; y<MAP_HEIGHT; ++y) for(int x=0; x<MAP_WIDTH; ++x) { map[y][x].type = WALL; map[y][x].rect = {(float)x*TILE_SIZE, (float)y*TILE_SIZE, (float)TILE_SIZE, (float)TILE_SIZE}; }
        std::vector<Rect> rooms;
        for(int i=0; i<15; ++i) {
            int w=6+rand()%6, h=6+rand()%6, x=1+rand()%(MAP_WIDTH-w-1), y=1+rand()%(MAP_HEIGHT-h-1);
            Rect r={(float)x, (float)y, (float)w, (float)h}; bool ok=true;
            for(const auto& ex:rooms) if(r.intersects({ex.x-1, ex.y-1, ex.w+2, ex.h+2})) { ok=false; break; }
            if(ok) { rooms.push_back(r); for(int ry=y; ry<y+h; ++ry) for(int rx=x; rx<x+w; ++rx) map[ry][rx].type=FLOOR; }
        }
        for(size_t i=1; i<rooms.size(); ++i) {
            Vec2 p1=rooms[i-1].center(), p2=rooms[i].center();
            int xDir=(p2.x>p1.x)?1:-1; for(int x=(int)p1.x; x!=(int)p2.x; x+=xDir) map[(int)p1.y][x].type=FLOOR;
            int yDir=(p2.y>p1.y)?1:-1; for(int y=(int)p1.y; y!=(int)p2.y; y+=yDir) map[y][(int)p2.x].type=FLOOR;
        }
        if(rooms.empty()) continue;
        std::vector<std::vector<bool>> reachable(MAP_HEIGHT, std::vector<bool>(MAP_WIDTH, false));
        std::queue<std::pair<int,int>> q; Vec2 s=rooms[0].center(); q.push({(int)s.x, (int)s.y}); reachable[(int)s.y][(int)s.x]=true;
        while(!q.empty()) { auto cur=q.front(); q.pop(); int dx[]={0,0,1,-1}, dy[]={1,-1,0,0}; for(int i=0; i<4; ++i) { int nx=cur.first+dx[i], ny=cur.second+dy[i]; if(nx>=0&&nx<MAP_WIDTH&&ny>=0&&ny<MAP_HEIGHT&&map[ny][nx].type==FLOOR && !reachable[ny][nx]) { reachable[ny][nx]=true; q.push({nx,ny}); } } }
        connected=true; for(int y=0; y<MAP_HEIGHT; ++y) for(int x=0; x<MAP_WIDTH; ++x) if(map[y][x].type==FLOOR && !reachable[y][x]) connected=false;
    }
}

void Game::handleInput() {
    input.update();
    if(input.isTriggered(SDL_SCANCODE_F1)) { debugMode=!debugMode; hud.addLog(debugMode?"DEV MODE: ON":"DEV MODE: OFF", COL_GOLD); }
    if(debugMode && input.isTriggered(SDL_SCANCODE_F2)) { sector++; init(); return; }
    if(input.isTriggered(SDL_SCANCODE_SPACE) && state==GameState::PLAYING) { p->reflexActive=!p->reflexActive; hud.addLog(p->reflexActive?"REFLEX: ON":"REFLEX: OFF"); }
    if(input.isPressed(SDL_SCANCODE_1)) currentAmmo=AmmoType::STANDARD; if(input.isPressed(SDL_SCANCODE_2)) currentAmmo=AmmoType::EMP; if(input.isPressed(SDL_SCANCODE_3)) currentAmmo=AmmoType::PIERCING;
    if(input.isTriggered(SDL_SCANCODE_R) && state==GameState::PLAYING) {
        int n = p->maxSlugs-p->slugs; if(n>0 && p->reserveSlugs>0) { int a = std::min(n, p->reserveSlugs); p->slugs+=a; p->reserveSlugs-=a; shake=5.0f; hud.addLog("WEAPON: Slugs reloaded."); }
    }
    if(input.isPressed(SDL_SCANCODE_F) && p->energy>50.0f) { p->energy-=50.0f; vfx.triggerFlash(0.5f); for(auto s:slugs) if(!s->isPlayer && s->pos.distance(p->pos)<250.0f) s->active=false; for(auto c:cores) if(c->pos.distance(p->pos)<200.0f) { c->stability-=150.0f; Vec2 d=(c->pos-p->pos).normalized(); if(d.length()<0.1f) d={0,-1}; c->vel=d*1200.0f; c->stunTimer=0.8f; } }
    if(input.isPressed(SDL_SCANCODE_LSHIFT) && p->energy>30.0f) { Vec2 d=p->vel.normalized(); if(d.length()<0.1f) d={0,-1}; p->vel=d*DASH_SPEED; p->dashTimer=0.15f; p->energy-=30.0f; }
    if(input.isPressed(SDL_SCANCODE_RETURN) && state!=GameState::PLAYING) init();
}

void Game::update() {
    if(state!=GameState::PLAYING) return; float dt=FRAME_DELAY/1000.0f; float ts=p->reflexActive?REFLEX_SCALE:1.0f; float wdt=dt*ts;
    if(shake>0) shake -= 20.0f*dt;
    if(p->dashTimer<=0) { Vec2 mv={0,0}; if(input.isPressed(SDL_SCANCODE_W)) mv.y=-1; if(input.isPressed(SDL_SCANCODE_S)) mv.y=1; if(input.isPressed(SDL_SCANCODE_A)) mv.x=-1; if(input.isPressed(SDL_SCANCODE_D)) mv.x=1; p->vel=mv.normalized()*PLAYER_SPEED; }
    
    Vec2 pCtr = p->bounds.center();
    Vec2 mWorld = input.mPos + cam;
    p->lookAngle = std::atan2(mWorld.y - pCtr.y, mWorld.x - pCtr.x);

    p->update(dt, map); objective.update(*this); hud.update(dt); if(debugMode) { p->suitIntegrity=100.0f; p->energy=100.0f; p->slugs=p->maxSlugs; }
    
    updatePickups(); updateWeapons(wdt); updateAI(wdt); updateSlugs(wdt); updateEchoes(wdt); 
    for(auto& ft : fTexts) { ft.pos.y -= 40.0f*dt; ft.life -= dt; }
    fTexts.erase(std::remove_if(fTexts.begin(), fTexts.end(), [](const FloatingText& t){ return t.life <= 0; }), fTexts.end());

    if(exit && exit->active && p->bounds.intersects(exit->bounds)) { sector++; SaveData sd={sector, score, p->suitIntegrity}; saveProgress(sd); init(); return; }
    Vec2 tCam=p->bounds.center()-Vec2(SCREEN_WIDTH/2, SCREEN_HEIGHT/2); cam.x+=(tCam.x-cam.x)*6.0f*dt; cam.y+=(tCam.y-cam.y)*6.0f*dt;
    if(shake >= 1.0f) { cam.x += (rand()%(int)shake) - (int)shake/2; cam.y += (rand()%(int)shake) - (int)shake/2; }
    lighting.update(p->bounds.center(), map); vfx.update(dt); if(p->suitIntegrity<=0) state=GameState::GAME_OVER;
}

void Game::updateWeapons(float dt) {
    if(input.mDown && p->shootCooldown<=0 && p->slugs>0) { 
        Vec2 d=(input.mPos+cam-p->bounds.center()).normalized(); 
        slugs.push_back(new KineticSlug(p->bounds.center(), d*800.0f, true, currentAmmo)); 
        p->shootCooldown=0.25f; p->slugs--; shake=3.0f;
    }
}

void Game::updatePickups() {
    for(auto i : items) {
        if(i->active && p->bounds.intersects(i->bounds)) {
            i->active = false;
            if(i->it == ItemType::REPAIR_KIT) { p->suitIntegrity = std::min(100.0f, p->suitIntegrity + 30.0f); spawnFText(i->pos, "REPAIRED", {50, 255, 50, 255}); }
            else if(i->it == ItemType::BATTERY_PACK) { p->reserveSlugs += 24; spawnFText(i->pos, "+24 SLUGS", COL_GOLD); }
            vfx.spawnBurst(i->pos, 10, COL_GOLD);
        }
    }
    items.erase(std::remove_if(items.begin(), items.end(), [](Item* i){ if(!i->active){ delete i; return true; } return false; }), items.end());
}

void Game::updateAI(float dt) {
    std::vector<RogueCore*> newSpawns;
    for (auto c : cores) { if (!c->active || c->sanitized) continue;
        
        Vec2 dirToPlayer = p->bounds.center() - c->bounds.center();
        c->lookAngle = std::atan2(dirToPlayer.y, dirToPlayer.x);

        RepairDrone* dr = dynamic_cast<RepairDrone*>(c); if (dr) { if (!dr->target) { float ms = 101.0f; for (auto tc : cores) if (tc->active && !tc->sanitized && !tc->contained && tc->stability < ms && tc->type == EntityType::ROGUE_CORE) { ms = tc->stability; dr->target = tc; } } if (dr->target) { Vec2 dir = (dr->target->pos - dr->pos); if (dir.length() < 40.0f) { dr->target->stability = std::min(100.0f, dr->target->stability + dr->repairPower * dt); dr->vel = {0,0}; } else dr->vel = dir.normalized() * 180.0f; } dr->update(dt, map); continue; }
        FinalBossCore* bs = dynamic_cast<FinalBossCore*>(c); if (bs && !bs->contained) { bs->phaseTimer += dt; if (bs->phase == 1 && bs->stability < 1000.0f) { bs->phase = 2; hud.addLog("BOSS: Shielding protocol engaged!", {255, 0, 255, 255}); } if (bs->phase == 2 && rand()%200 == 0) newSpawns.push_back(new SeekerSwarm(bs->bounds.center())); }
        if (c->contained) continue; if (c->stunTimer > 0) { c->stunTimer -= dt; c->vel = c->vel * std::pow(0.1f, dt); c->update(dt, map); continue; }
        float d = c->bounds.center().distance(p->bounds.center()); if (d < 400) { c->stateTimer -= dt; if (c->stateTimer <= 0) { c->calculatePath(p->bounds.center(), map); c->stateTimer = 0.5f; } if (!c->path.empty() && c->pathIndex < c->path.size()) { Vec2 dir = (c->path[c->pathIndex] - c->bounds.center()); if (dir.length() < 10.0f) c->pathIndex++; else c->vel = dir.normalized() * AI_SPEED; } }
        if (d < 250 && rand()%100 < 2) slugs.push_back(new KineticSlug(c->bounds.center(), (p->bounds.center()-c->bounds.center()).normalized()*450.0f, false)); c->update(dt, map);
    }
    for(auto n : newSpawns) cores.push_back(n);
    cores.erase(std::remove_if(cores.begin(), cores.end(), [](RogueCore* c){ if(!c->active){ delete c; return true; } return false; }), cores.end());
    for(auto c : cores) if(c->contained && !c->sanitized && p->bounds.intersects(c->bounds)) { c->sanitized = true; score += 150; spawnFText(c->pos, "SANITIZED", COL_PLAYER); vfx.spawnBurst(c->pos, 25, COL_PLAYER); }
}

void Game::updateSlugs(float dt) { for (auto s : slugs) { if (!s->active) continue; s->update(dt, map); if (s->isPlayer) { for (auto c : cores) if (c->active && !c->contained && s->bounds.intersects(c->bounds)) { float dmg = 25.0f * s->powerMultiplier; if (s->ammoType == AmmoType::EMP) { c->stunTimer = 1.2f; dmg *= 0.5f; } if (s->ammoType == AmmoType::PIERCING) dmg *= 1.5f; GuardianCore* g = dynamic_cast<GuardianCore*>(c); if (g && g->shield > 0) { g->shield -= dmg; if (s->ammoType != AmmoType::PIERCING) s->active = false; vfx.spawnBurst(s->pos, 5, {100, 200, 255, 255}); } else { c->stability -= dmg; s->active = false; vfx.spawnBurst(s->pos, 8, COL_SLUG); if (c->stability <= 0) { c->contained = true; c->vel = {0,0}; score += 50; } } } } else if (s->bounds.intersects(p->bounds)) { p->suitIntegrity -= 10.0f; s->active = false; vfx.spawnBurst(s->pos, 5, COL_PLAYER); shake = 8.0f; } } slugs.erase(std::remove_if(slugs.begin(), slugs.end(), [](KineticSlug* s) { if(!s->active) { delete s; return true; } return false; }), slugs.end()); }
void Game::updateEchoes(float dt) { if (state == GameState::PLAYING && rand()%1000 < 1 + sector) echoes.push_back(new NeuralEcho(p->pos + Vec2((float)(rand()%400-200), (float)(rand()%400-200)))); for (auto e : echoes) { e->vel = (p->pos - e->pos).normalized() * 100.0f; e->update(dt, map); if (e->active && e->bounds.intersects(p->bounds)) { p->suitIntegrity -= 15.0f; e->active = false; vfx.triggerFlash(0.3f); } } echoes.erase(std::remove_if(echoes.begin(), echoes.end(), [](NeuralEcho* e) { if(!e->active) { delete e; return true; } return false; }), echoes.end()); }
void Game::spawnFText(Vec2 pos, std::string t, SDL_Color c) { fTexts.push_back({pos, t, 1.0f, c}); }

void Game::render() {
    SDL_SetRenderDrawColor(ren, COL_BG.r, COL_BG.g, COL_BG.b, 255); SDL_RenderClear(ren); if (state == GameState::MENU) { renderT("RECOIL PROTOCOL", SCREEN_WIDTH/2-180, 180, fontL, COL_PLAYER); renderT("Press ENTER to Start", SCREEN_WIDTH/2-100, 350, font, COL_TEXT); } else if (state == GameState::PLAYING) { int sx = std::max(0, (int)(cam.x/TILE_SIZE)), sy = std::max(0, (int)(cam.y/TILE_SIZE)); int ex = std::min(MAP_WIDTH, (int)((cam.x+SCREEN_WIDTH)/TILE_SIZE)+1), ey = std::min(MAP_HEIGHT, (int)((cam.y+SCREEN_HEIGHT)/TILE_SIZE)+1); for (int y=sy; y<ey; ++y) for (int x=sx; x<ex; ++x) { SDL_Rect r = {(int)(x*TILE_SIZE-cam.x), (int)(y*TILE_SIZE-cam.y), TILE_SIZE, TILE_SIZE}; SDL_SetRenderDrawColor(ren, (map[y][x].type==WALL)?COL_WALL.r:COL_FLOOR.r, (map[y][x].type==WALL)?COL_WALL.g:COL_FLOOR.g, (map[y][x].type==WALL)?COL_WALL.b:COL_FLOOR.b, 255); SDL_RenderFillRect(ren, &r); if (map[y][x].type==WALL) { SDL_SetRenderDrawColor(ren, 50, 50, 100, 255); SDL_RenderDrawRect(ren, &r); } } 
    if (exit) { 
        SDL_Rect er = {(int)(exit->pos.x-cam.x), (int)(exit->pos.y-cam.y), 40, 40};
        if (exit->active) { 
            SDL_SetRenderDrawColor(ren, 100, 255, 100, (Uint8)(150+std::sin(SDL_GetTicks()*0.01f)*100)); SDL_RenderFillRect(ren, &er); SDL_SetRenderDrawColor(ren, 255, 255, 255, 255); SDL_RenderDrawRect(ren, &er); 
            renderT("EXTRACTION POINT", er.x - 20, er.y - 25, font, {100, 255, 100, 255});
        } else { SDL_SetRenderDrawColor(ren, 40, 40, 80, 100); SDL_RenderFillRect(ren, &er); SDL_SetRenderDrawColor(ren, 150, 50, 50, 255); SDL_RenderDrawRect(ren, &er); renderT("EXIT LOCKED", er.x - 10, er.y - 25, font, {150, 50, 50, 255}); }
    } 
    for (auto c : cores) c->render(ren, cam); p->render(ren, cam); for (auto s : slugs) s->render(ren, cam); for (auto i : items) i->render(ren, cam); for (auto e : echoes) e->render(ren, cam);
    for (const auto& ft : fTexts) { renderT(ft.text, (int)(ft.pos.x-cam.x), (int)(ft.pos.y-cam.y), font, ft.color); }
    vfx.render(ren, cam); lighting.render(ren, cam); hud.render(ren, p, score, sector, *this, font, fontL); } else { renderT("PROTOCOL FAILURE", SCREEN_WIDTH/2-180, 200, fontL, {255, 50, 50, 255}); renderT("Press ENTER to Reboot", SCREEN_WIDTH/2-100, 400, font, COL_TEXT); } SDL_RenderPresent(ren);
}
void Game::renderT(std::string t, int x, int y, TTF_Font* f, SDL_Color c) { if (!f || t.empty()) return; SDL_Surface* s = TTF_RenderText_Blended(f, t.c_str(), c); if (s) { SDL_Texture* tx = SDL_CreateTextureFromSurface(ren, s); SDL_Rect d = {x, y, s->w, s->h}; SDL_RenderCopy(ren, tx, NULL, &d); SDL_DestroyTexture(tx); SDL_FreeSurface(s); } }
void Game::loop() { while (running) { Uint32 st = SDL_GetTicks(); handleInput(); update(); render(); Uint32 t = SDL_GetTicks()-st; if (t<FRAME_DELAY) SDL_Delay((Uint32)FRAME_DELAY - t); } }
int main(int a, char** b) { (void)a; (void)b; srand(time(NULL)); Game g; g.loop(); return 0; }