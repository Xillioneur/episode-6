/*
 * Recoil Protocol: System Glitch Edition - Episode 6
 * ----------------------------------------------------------------------------
 * A unique Top-Down Tactical Action Game.
 * Theme: Cybernetic Security Inspector
 *
 * Unique Mechanics:
 * - KINETIC IMPACTORS: Slugs gain power with each wall bounce.
 * - NEURAL ECHOES: Supernatural system glitches that ignore walls.
 * - TACTICAL DASH: High-speed repositioning using Suit Energy.
 * - BOSS CORES: High-stability Guardians with protective shields.
 *
 * Compile:
 * g++ main.cpp -o recoilprotocol -std=c++17 -lSDL2 -lSDL2_ttf
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
// [CONSTANTS & CONFIGURATION]
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

// Neon Safety Palette
const SDL_Color COL_BG = {5, 5, 10, 255};
const SDL_Color COL_WALL = {35, 40, 55, 255};
const SDL_Color COL_FLOOR = {15, 15, 20, 255};
const SDL_Color COL_PLAYER = {50, 255, 150, 255}; // Neon Mint
const SDL_Color COL_CORE = {255, 80, 50, 255};    // Warning Red
const SDL_Color COL_SLUG = {255, 255, 255, 255};  // Kinetic Slugs (Player)
const SDL_Color COL_ROGUE_SLUG = {255, 150, 50, 255}; // Hazard Orange (Enemy)
const SDL_Color COL_GLITCH = {200, 50, 255, 180}; // Purple Echoes
const SDL_Color COL_TEXT = {220, 230, 255, 255};

// ============================================================================
// [MATH UTILITIES]
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
    Vec2 normalized() const { float l = length(); return (l > 0.0001f) ? Vec2(x / l, y / l) : Vec2(0, 0); }
    float distance(const Vec2& v) const { return (*this - v).length(); }
};

struct Rect {
    float x, y, w, h;
    bool intersects(const Rect& other) const { return (x < other.x + other.w && x + w > other.x && y < other.y + other.h && y + h > other.y); }
    Vec2 center() const { return {x + w / 2, y + h / 2}; }
};

// ============================================================================
// [GAME ENGINE COMPONENTS]
// ============================================================================

enum class GameState { MENU, PLAYING, GAME_OVER };

struct Particle {
    Vec2 pos; Vec2 vel;
    float life; float maxLife;
    SDL_Color color; float size;
};

enum TileType { WALL, FLOOR };
struct Tile { TileType type; Rect rect; };

enum class EntityType { PLAYER, ROGUE_CORE, NEURAL_ECHO, KINETIC_SLUG, ITEM, EXIT, HAZARD, DECORATION, GADGET };

// Forward declarations for Game class and methods
class Game;

// ============================================================================
// [BASE ENTITY CLASS]
// ============================================================================

class Entity {
public:
    Vec2 pos;
    Vec2 vel;
    Rect bounds;
    EntityType type;
    bool active = true;
    float angle = 0.0f; 

    Entity(Vec2 p, float w, float h, EntityType t) : pos(p), type(t) {
        bounds = {p.x, p.y, w, h};
    }
    virtual ~Entity() {}

    virtual void update(float dt, const std::vector<std::vector<Tile>>& map) {
        move(vel * dt, map);
    }

    void move(Vec2 delta, const std::vector<std::vector<Tile>>& map) {
        float dist = delta.length();
        if (dist <= 0) return;
        
        int steps = (int)(dist / 8.0f) + 1;
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
        bounds.x = pos.x;
        bounds.y = pos.y;
    }

    virtual void render(SDL_Renderer* renderer, const Vec2& camera) {
        SDL_Rect r = {(int)(pos.x - camera.x), (int)(pos.y - camera.y), (int)bounds.w, (int)bounds.h};
        if (type == EntityType::PLAYER) SDL_SetRenderDrawColor(renderer, COL_PLAYER.r, COL_PLAYER.g, COL_PLAYER.b, 255);
        else if (type == EntityType::ROGUE_CORE) SDL_SetRenderDrawColor(renderer, COL_CORE.r, COL_CORE.g, COL_CORE.b, 255);
        else SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderFillRect(renderer, &r);
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
                        if (moveDir > 0) pos.x = map[y][x].rect.x - bounds.w - 0.001f;
                        else if (moveDir < 0) pos.x = map[y][x].rect.x + map[y][x].rect.w + 0.001f;
                        vel.x = 0;
                    } else {
                        if (moveDir > 0) pos.y = map[y][x].rect.y - bounds.h - 0.001f;
                        else if (moveDir < 0) pos.y = map[y][x].rect.y + map[y][x].rect.h + 0.001f;
                        vel.y = 0;
                    }
                    bounds.x = pos.x;
                    bounds.y = pos.y;
                }
            }
        }
    }
};

// ============================================================================
// [DECORATIONS & GADGETS]
// ============================================================================

class Decoration : public Entity {
public:
    Decoration(Vec2 p, float w, float h) : Entity(p, w, h, EntityType::DECORATION) {}
};

class WallPipe : public Decoration {
public:
    bool horizontal;
    WallPipe(Vec2 p, bool horiz) : Decoration(p, horiz?40:10, horiz?10:40), horizontal(horiz) {}
    void render(SDL_Renderer* ren, const Vec2& cam) override {
        SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        SDL_SetRenderDrawColor(ren, 70, 75, 90, 255);
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren, 100, 110, 130, 255);
        if (horizontal) SDL_RenderDrawLine(ren, r.x, r.y + 2, r.x + r.w, r.y + 2);
        else SDL_RenderDrawLine(ren, r.x + 2, r.y, r.x + 2, r.y + r.h);
    }
};

class FloorCable : public Decoration {
public:
    Vec2 p2;
    FloorCable(Vec2 _p1, Vec2 _p2) : Decoration(_p1, 0, 0), p2(_p2) {}
    void render(SDL_Renderer* ren, const Vec2& cam) override {
        SDL_SetRenderDrawColor(ren, 30, 30, 40, 255);
        SDL_RenderDrawLine(ren, (int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)(p2.x - cam.x), (int)(p2.y - cam.y));
    }
};

class ProximitySensor : public Entity {
public:
    float life = 15.0f;
    ProximitySensor(Vec2 p) : Entity(p, 10, 10, EntityType::GADGET) {}
    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
        (void)map;
        life -= dt; if (life <= 0) active = false;
    }
    void render(SDL_Renderer* ren, const Vec2& cam) override {
        SDL_Rect r = {(int)(pos.x - cam.x - 5), (int)(pos.y - cam.y - 5), 10, 10};
        SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
        SDL_RenderFillRect(ren, &r);
        float s = 20.0f + std::abs(std::sin(SDL_GetTicks()*0.005f)) * 40.0f;
        SDL_Rect ring = {(int)(pos.x - cam.x - s/2), (int)(pos.y - cam.y - s/2), (int)s, (int)s};
        SDL_RenderDrawRect(ren, &ring);
    }
};

// ============================================================================
// [DERIVED ENTITIES]
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

    Player(Vec2 p) : Entity(p, 20, 20, EntityType::PLAYER) {}
    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
        if (dashTimer > 0) dashTimer -= dt;
        else Entity::update(dt, map);
        if (shootCooldown > 0) shootCooldown -= dt;
        energy = std::min(100.0f, energy + 12.0f * dt);
        if (reflexActive) {
            reflexMeter -= 25.0f * dt;
            if (reflexMeter <= 0) { reflexMeter = 0; reflexActive = false; }
        } else {
            reflexMeter = std::min(100.0f, reflexMeter + 15.0f * dt);
        }
    }
};

class RogueCore : public Entity {
public:
    float stability = 100.0f;
    bool contained = false;
    bool sanitized = false;
    float stateTimer = 0.0f;
    std::vector<Vec2> path;
    size_t pathIndex = 0;
    float shootTimer = 0.0f;

    RogueCore(Vec2 p) : Entity(p, 24, 24, EntityType::ROGUE_CORE) { vel = {0,0}; }

    void render(SDL_Renderer* renderer, const Vec2& camera) override {
        SDL_Rect r = {(int)(pos.x - camera.x), (int)(pos.y - camera.y), (int)bounds.w, (int)bounds.h};
        if (sanitized) {
            SDL_SetRenderDrawColor(renderer, 40, 40, 55, 255);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
            SDL_RenderDrawRect(renderer, &r);
        } else if (contained) {
            SDL_SetRenderDrawColor(renderer, 80, 150, 255, 255);
            SDL_RenderDrawRect(renderer, &r);
            float pulse = std::sin(SDL_GetTicks()*0.01f) * 4.0f;
            SDL_Rect inner = {r.x + 6 + (int)pulse/2, r.y + 6 + (int)pulse/2, 12 - (int)pulse, 12 - (int)pulse};
            SDL_RenderDrawRect(renderer, &inner);
        } else {
            SDL_SetRenderDrawColor(renderer, COL_CORE.r, COL_CORE.g, COL_CORE.b, 255);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect core = {r.x + 8, r.y + 8, 8, 8}; SDL_RenderFillRect(renderer, &core);
        }
    }

    void calculatePath(Vec2 target, const std::vector<std::vector<Tile>>& map);
};

class GuardianCore : public RogueCore {
public:
    float shield = 200.0f;
    float phaseTimer = 0.0f;
    int phase = 0;
    GuardianCore(Vec2 p) : RogueCore(p) { bounds.w = 48; bounds.h = 48; stability = 500.0f; }
    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
        RogueCore::update(dt, map);
        phaseTimer += dt;
        if (phaseTimer > 3.0f) { phaseTimer = 0; phase = (phase + 1) % 3; }
    }
    void render(SDL_Renderer* ren, const Vec2& cam) override {
        RogueCore::render(ren, cam);
        if (shield > 0) {
            SDL_Rect r = {(int)(pos.x - cam.x - 4), (int)(pos.y - cam.y - 4), (int)bounds.w + 8, (int)bounds.h + 8};
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 100, 200, 255, 120);
            SDL_RenderDrawRect(ren, &r);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        }
    }
};

class NeuralEcho : public Entity {
public:
    float life = 4.0f;
    NeuralEcho(Vec2 p) : Entity(p, 30, 30, EntityType::NEURAL_ECHO) {}
    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
        (void)map;
        life -= dt; if (life <= 0) active = false;
    }
    void render(SDL_Renderer* ren, const Vec2& camera) override {
        SDL_Rect r = {(int)(pos.x - camera.x), (int)(pos.y - camera.y), (int)bounds.w, (int)bounds.h};
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, COL_GLITCH.r, COL_GLITCH.g, COL_GLITCH.b, (Uint8)(100 + std::sin(SDL_GetTicks()*0.01f)*50));
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }
};

class KineticSlug : public Entity {
public:
    int bounces = 4;
    float powerMultiplier = 1.0f;
    bool isPlayer;
    std::vector<Vec2> tail;

    KineticSlug(Vec2 p, Vec2 v, bool pOwned) : Entity(p, 6, 6, EntityType::KINETIC_SLUG), isPlayer(pOwned) { vel = v; }
    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
        tail.push_back(pos); if (tail.size() > 12) tail.erase(tail.begin());
        Vec2 delta = vel * dt;
        float dist = delta.length();
        int steps = (int)(dist / 4.0f) + 1; Vec2 step = delta / (float)steps;
        for (int i = 0; i < steps; ++i) {
            pos.x += step.x; if (checkWall(map)) { pos.x -= step.x; vel.x *= -1; handleBounce(); }
            pos.y += step.y; if (checkWall(map)) { pos.y -= step.y; vel.y *= -1; handleBounce(); }
        }
        bounds.x = pos.x; bounds.y = pos.y;
        if (pos.x < 0 || pos.y < 0 || pos.x > MAP_WIDTH*TILE_SIZE || pos.y > MAP_HEIGHT*TILE_SIZE) active = false;
    }
    bool checkWall(const std::vector<std::vector<Tile>>& map) {
        int bx = (int)(pos.x / TILE_SIZE), by = (int)(pos.y / TILE_SIZE);
        return (bx >= 0 && bx < MAP_WIDTH && by >= 0 && by < MAP_HEIGHT && map[by][bx].type == WALL);
    }
    void handleBounce() { bounces--; powerMultiplier += 0.6f; if (bounces < 0) active = false; }
    void render(SDL_Renderer* renderer, const Vec2& camera) override {
        SDL_Color trailCol = isPlayer ? SDL_Color{150, 255, 255, 255} : COL_ROGUE_SLUG;
        for (size_t i = 0; i < tail.size(); ++i) {
            float alphaMod = (i / (float)tail.size());
            SDL_SetRenderDrawColor(renderer, trailCol.r, trailCol.g, trailCol.b, (Uint8)(60 * alphaMod));
            SDL_Rect tr = {(int)(tail[i].x - camera.x), (int)(tail[i].y - camera.y), 4, 4};
            SDL_RenderFillRect(renderer, &tr);
        }
        if (isPlayer) SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        else SDL_SetRenderDrawColor(renderer, COL_ROGUE_SLUG.r, COL_ROGUE_SLUG.g, COL_ROGUE_SLUG.b, 255);
        SDL_Rect r = {(int)(pos.x - camera.x), (int)(pos.y - camera.y), (int)bounds.w, (int)bounds.h};
        SDL_RenderFillRect(renderer, &r);
    }
};

// ============================================================================
// [ENVIRONMENTAL HAZARDS]
// ============================================================================

class Hazard : public Entity {
public:
    Hazard(Vec2 p, float w, float h) : Entity(p, w, h, EntityType::HAZARD) {}
    virtual void onTrigger(class Game& game) = 0;
};

class ExplosiveCell : public Hazard {
public:
    float fuse = 0.6f;
    bool triggered = false;
    ExplosiveCell(Vec2 p) : Hazard(p, 30, 30) {}
    void render(SDL_Renderer* ren, const Vec2& cam) override {
        SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        if (triggered && (int)(SDL_GetTicks()/100)%2 == 0) SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        else SDL_SetRenderDrawColor(ren, 255, 120, 0, 255);
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderDrawRect(ren, &r);
    }
    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
        (void)map;
        if (triggered) { fuse -= dt; if (fuse <= 0) active = false; }
    }
    void onTrigger(class Game& game) override;
};

class SecurityLaser : public Hazard {
public:
    float timer = 0.0f;
    bool isOn = true;
    Vec2 p1, p2;
    SecurityLaser(Vec2 _p1, Vec2 _p2) : Hazard(_p1, 0, 0), p1(_p1), p2(_p2) {}
    void update(float dt, const std::vector<std::vector<Tile>>& map) override {
        (void)map;
        timer += dt; if (timer > 2.5f) { timer = 0; isOn = !isOn; }
    }
    void render(SDL_Renderer* ren, const Vec2& cam) override {
        if (!isOn) return;
        SDL_SetRenderDrawColor(ren, 255, 50, 50, 180);
        SDL_RenderDrawLine(ren, (int)(p1.x - cam.x), (int)(p1.y - cam.y), (int)(p2.x - cam.x), (int)(p2.y - cam.y));
    }
    void onTrigger(class Game& game) override;
};

// ============================================================================
// [ITEMS & PICKUPS]
// ============================================================================

enum class ItemType { REPAIR_KIT, BATTERY_PACK, COOLANT, OVERCLOCK };
class Item : public Entity {
public:
    ItemType it;
    Item(Vec2 p, ItemType _it) : Entity(p, 18, 18, EntityType::ITEM), it(_it) {}
    void render(SDL_Renderer* r, const Vec2& cam) override {
        SDL_Rect dr = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
        switch(it) {
            case ItemType::REPAIR_KIT: SDL_SetRenderDrawColor(r, 0, 255, 100, 255); break;
            case ItemType::BATTERY_PACK: SDL_SetRenderDrawColor(r, 255, 255, 0, 255); break;
            case ItemType::COOLANT: SDL_SetRenderDrawColor(r, 0, 150, 255, 255); break;
            case ItemType::OVERCLOCK: SDL_SetRenderDrawColor(r, 255, 100, 0, 255); break;
        }
        SDL_RenderFillRect(r, &dr);
    }
};

// ============================================================================
// [AUDIO & SOUND SYSTEM]
// ============================================================================

class SoundManager {
public:
    SoundManager() {
        // SDL_mixer initialization would go here in a real project
    }
    void playSlugFire() { /* Play low-latency beep */ }
    void playImpact() { /* Play kinetic thud */ }
    void playAlert() { /* Play system alarm */ }
};

// ============================================================================
// [SAVE & PERSISTENCE]
// ============================================================================

struct SaveData {
    int sector;
    int score;
    float integrity;
};

void saveProgress(const SaveData& data) {
    FILE* f = fopen("recoil_save.bin", "wb");
    if (f) { fwrite(&data, sizeof(SaveData), 1, f); fclose(f); }
}

bool loadProgress(SaveData& data) {
    FILE* f = fopen("recoil_save.bin", "rb");
    if (f) { fread(&data, sizeof(SaveData), 1, f); fclose(f); return true; }
    return false;
}

// ... (HUD Class definition)
class HUD {
public:
    struct LogEntry { std::string msg; float life; SDL_Color col; };
    std::vector<LogEntry> logs;
    void addLog(const std::string& m, SDL_Color c = {200, 200, 255, 255});
    void update(float dt);
    void render(SDL_Renderer* ren, Player* p, int score, int sector, bool overclocked, TTF_Font* font, TTF_Font* fontL, Game& game);
    void renderText(SDL_Renderer* ren, const std::string& t, int x, int y, TTF_Font* f, SDL_Color c);
private:
    void drawBar(SDL_Renderer* ren, int x, int y, int w, int h, float pct, SDL_Color col);
};

// ============================================================================
// [GAME CLASS]
// ============================================================================

class Game {
public:
    bool running = true; GameState state = GameState::MENU;
    SDL_Window* win = nullptr; SDL_Renderer* ren = nullptr;
    TTF_Font *font = nullptr, *fontL = nullptr;

    struct FloatingText { Vec2 pos; std::string text; float life; SDL_Color color; };
    std::vector<FloatingText> fTexts;
    float hitstop = 0.0f;

    bool keys[SDL_NUM_SCANCODES] = {false}; bool mDown = false; Vec2 mPos;
    std::vector<std::vector<Tile>> map; std::vector<std::vector<float>> lMap;

    Player* p = nullptr; 
    std::vector<RogueCore*> cores; 
    std::vector<KineticSlug*> slugs;
    std::vector<NeuralEcho*> echoes; 
    std::vector<Particle> parts; 
    std::vector<Item*> items; 
    std::vector<Hazard*> hazards;
    std::vector<Entity*> decos;
    Entity* exit = nullptr;
    
    HUD hud;
    SoundManager sound;
    Vec2 cam = {0, 0}; float shake = 0.0f;
    int score = 0, sector = 1; bool overclocked = false;
    
    bool isAmbush = false;
    std::string ambushMsg = "";
    float ambushDuration = 0.0f;

    Game();
    ~Game();

    void init();
    void cleanup();
    void generateLevel();
    Vec2 findSpace();
    
    void handleInput();
    void update();
    void render();
    void renderT(std::string t, int x, int y, TTF_Font* f, SDL_Color c);
    
    void updateAI(float dt);
    void updateSlugs(float dt);
    void updateEchoes(float dt);
    void updateHazards(float dt);
    void updateParts(float dt);
    void updateLight();
    
    void spawnParts(Vec2 p, int n, SDL_Color c);
    void loop();
};

// ============================================================================
// [METHOD IMPLEMENTATIONS]
// ============================================================================

void RogueCore::calculatePath(Vec2 target, const std::vector<std::vector<Tile>>& map) {
    int sx = (int)(bounds.center().x/TILE_SIZE), sy = (int)(bounds.center().y/TILE_SIZE);
    int ex = (int)(target.x/TILE_SIZE), ey = (int)(target.y/TILE_SIZE);
    if (sx == ex && sy == ey) { path.clear(); return; }
    if (ex < 0 || ex >= MAP_WIDTH || ey < 0 || ey >= MAP_HEIGHT || map[ey][ex].type == WALL) return;
    static float gS[MAP_HEIGHT][MAP_WIDTH]; static std::pair<int,int> par[MAP_HEIGHT][MAP_WIDTH]; static bool vis[MAP_HEIGHT][MAP_WIDTH];
    for(int y=0; y<MAP_HEIGHT; ++y) for(int x=0; x<MAP_WIDTH; ++x) { gS[y][x] = 1e6f; vis[y][x] = false; }
    std::priority_queue<std::pair<float, std::pair<int,int>>, std::vector<std::pair<float, std::pair<int,int>>>, std::greater<std::pair<float, std::pair<int,int>>>> pq;
    gS[sy][sx] = 0; pq.push({0, {sx, sy}});
    bool found = false;
    while(!pq.empty()) {
        auto cur = pq.top().second; pq.pop(); int cx = cur.first, cy = cur.second;
        if (cx == ex && cy == ey) { found = true; break; }
        if (vis[cy][cx]) continue; 
        vis[cy][cx] = true;
        int dx[] = {0,0,1,-1}, dy[] = {1,-1,0,0};
        for(int i=0; i<4; ++i) {
            int nx = cx+dx[i], ny = cy+dy[i];
            if (nx>=0 && nx<MAP_WIDTH && ny>=0 && ny<MAP_HEIGHT && map[ny][nx].type != WALL && !vis[ny][nx]) {
                float tg = gS[cy][cx] + 1.0f;
                if (tg < gS[ny][nx]) { par[ny][nx] = {cx, cy}; gS[ny][nx] = tg; pq.push({tg + (float)std::abs(nx-ex)+(float)std::abs(ny-ey), {nx, ny}}); }
            }
        }
    }
    path.clear(); if (found) { int cx = ex, cy = ey; while (cx != sx || cy != sy) { path.push_back({(float)cx*TILE_SIZE+20, (float)cy*TILE_SIZE+20}); auto p = par[cy][cx]; cx=p.first; cy=p.second; } std::reverse(path.begin(), path.end()); pathIndex = 0; }
}

void ExplosiveCell::onTrigger(Game& game) { 
    (void)game;
    if (triggered) return; 
    triggered = true; 
}
void SecurityLaser::onTrigger(Game& game) { (void)game; }

void HUD::addLog(const std::string& m, SDL_Color c) {
    logs.push_back({m, 5.0f, c});
    if (logs.size() > 6) logs.erase(logs.begin());
}

void HUD::update(float dt) {
    for (auto& l : logs) l.life -= dt;
    logs.erase(std::remove_if(logs.begin(), logs.end(), [](const LogEntry& le) { return le.life <= 0; }), logs.end());
}

void HUD::drawBar(SDL_Renderer* ren, int x, int y, int w, int h, float pct, SDL_Color col) {
    SDL_Rect bg = {x, y, w, h}; SDL_SetRenderDrawColor(ren, 40, 40, 40, 255); SDL_RenderFillRect(ren, &bg);
    SDL_Rect fg = {x, y, (int)(w * std::clamp(pct, 0.0f, 1.0f)), h}; SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a); SDL_RenderFillRect(ren, &fg);
    SDL_SetRenderDrawColor(ren, 80, 80, 80, 255); SDL_RenderDrawRect(ren, &bg);
}

void HUD::renderText(SDL_Renderer* ren, const std::string& t, int x, int y, TTF_Font* f, SDL_Color c) {
    if (!f || t.empty()) return; 
    SDL_Surface* s = TTF_RenderText_Blended(f, t.c_str(), c);
    if (s) { SDL_Texture* tx = SDL_CreateTextureFromSurface(ren, s); SDL_Rect dst = {x, y, s->w, s->h}; SDL_RenderCopy(ren, tx, NULL, &dst); SDL_DestroyTexture(tx); SDL_FreeSurface(s); }
}

void HUD::render(SDL_Renderer* ren, Player* p, int score, int sector, bool overclocked, TTF_Font* font, TTF_Font* fontL, Game& game) {
    drawBar(ren, 20, 20, 200, 18, p->suitIntegrity / 100.0f, {50, 255, 100, 255});
    renderText(ren, "SUIT INTEGRITY", 25, 21, font, {255, 255, 255, 255});
    drawBar(ren, 20, 45, 150, 10, p->energy / 100.0f, {50, 150, 255, 255});
    renderText(ren, "ENERGY", 25, 45, font, {200, 200, 255, 255});
    drawBar(ren, 20, 60, 150, 10, p->reflexMeter / 100.0f, {255, 200, 50, 255});
    renderText(ren, "REFLEX", 25, 60, font, {255, 255, 200, 255});
    std::string slugsText = "SLUGS: " + std::to_string(p->slugs) + " / " + std::to_string(p->reserveSlugs);
    renderText(ren, slugsText, 20, 85, font, {220, 220, 220, 255});
    if (overclocked) {
        float pulse = std::abs(std::sin(SDL_GetTicks()*0.01f));
        renderText(ren, ">>> OVERCLOCK ACTIVE <<<", 20, 105, font, {(Uint8)(255*pulse), 100, 0, 255});
    }
    renderText(ren, "SECTOR: " + std::to_string(sector), SCREEN_WIDTH - 160, 20, font, {150, 150, 255, 255});
    renderText(ren, "SCORE:  " + std::to_string(score), SCREEN_WIDTH - 160, 40, font, {255, 255, 255, 255});
    int logY = SCREEN_HEIGHT - 120;
    for (auto& l : logs) {
        Uint8 alpha = (Uint8)(255 * (l.life / 5.0f));
        SDL_Color cl = l.col; cl.a = alpha;
        renderText(ren, "> " + l.msg, 20, logY, font, cl);
        logY += 18;
    }
    SDL_Rect mmRect = {SCREEN_WIDTH - 110, 70, 100, 100};
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 150); SDL_RenderFillRect(ren, &mmRect);
    SDL_SetRenderDrawColor(ren, 50, 50, 100, 255); SDL_RenderDrawRect(ren, &mmRect);
    SDL_Rect pDot = {mmRect.x + 50 + (int)(p->pos.x / 200), mmRect.y + 50 + (int)(p->pos.y / 200), 3, 3};
    SDL_SetRenderDrawColor(ren, 0, 255, 0, 255); SDL_RenderFillRect(ren, &pDot);
    if (game.isAmbush) {
        SDL_SetRenderDrawColor(ren, 255, 0, 0, 80); SDL_Rect full = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}; SDL_RenderFillRect(ren, &full);
        renderText(ren, game.ambushMsg, SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2, fontL, {255, 255, 255, 255});
    }
}

Game::Game() {
    SDL_Init(SDL_INIT_VIDEO); TTF_Init();
    win = SDL_CreateWindow("Recoil Protocol", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
#ifdef __APPLE__
    font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 18); fontL = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 52);
#else
    font = TTF_OpenFont("arial.ttf", 18); fontL = TTF_OpenFont("arial.ttf", 52);
#endif
    init();
}

Game::~Game() { cleanup(); if (font) TTF_CloseFont(font); if (fontL) TTF_CloseFont(fontL); SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); TTF_Quit(); SDL_Quit(); }

void Game::init() {
    cleanup(); generateLevel();
    lMap.resize(MAP_HEIGHT); for(auto& r : lMap) r.assign(MAP_WIDTH, 0.0f);
    p = new Player(findSpace()); p->reserveSlugs = 60; overclocked = false;
    for(int i=0; i<5 + sector*2; ++i) cores.push_back(new RogueCore(findSpace()));
    if (sector % 3 == 0) cores.push_back(new GuardianCore(findSpace()));
    for(int i=0; i<8; ++i) { 
        int r = rand()%100; ItemType it = (r<40)?ItemType::BATTERY_PACK:(r<70)?ItemType::REPAIR_KIT:(r<90)?ItemType::COOLANT:ItemType::OVERCLOCK;
        items.push_back(new Item(findSpace(), it));
    }
    for(int i=0; i<6; ++i) hazards.push_back(new ExplosiveCell(findSpace()));
    exit = new Entity(findSpace(), 40, 40, EntityType::EXIT); exit->active = false;
    state = GameState::PLAYING;
    hud.addLog("SYSTEM INITIALIZED. SECTOR " + std::to_string(sector));
}

void Game::cleanup() {
    if (p) delete p; for (auto c : cores) delete c; for (auto s : slugs) delete s; for (auto e : echoes) delete e; 
    for (auto i : items) delete i; for (auto h : hazards) delete h; for (auto d : decos) delete d; if (exit) delete exit;
    cores.clear(); slugs.clear(); echoes.clear(); items.clear(); hazards.clear(); decos.clear(); parts.clear(); fTexts.clear();
}

Vec2 Game::findSpace() {
    int x, y, at = 0; do { x = 1 + rand()%(MAP_WIDTH-2); y = 1 + rand()%(MAP_HEIGHT-2); at++; if (at>500) break; } while (map[y][x].type == WALL);
    return {(float)x*TILE_SIZE+20, (float)y*TILE_SIZE+20};
}

void Game::generateLevel() {
    map.resize(MAP_HEIGHT); for (auto& r : map) r.resize(MAP_WIDTH);
    for (int y=0; y<MAP_HEIGHT; ++y) for (int x=0; x<MAP_WIDTH; ++x) { map[y][x].type = WALL; map[y][x].rect = {(float)x*TILE_SIZE, (float)y*TILE_SIZE, (float)TILE_SIZE, (float)TILE_SIZE}; }
    std::vector<Rect> rooms;
    for (int i=0; i<15; ++i) {
        int w = 6+rand()%6, h = 6+rand()%6, x = 1+rand()%(MAP_WIDTH-w-1), y = 1+rand()%(MAP_HEIGHT-h-1);
        Rect r = {(float)x, (float)y, (float)w, (float)h}; bool ok = true;
        for (const auto& ex : rooms) if (r.intersects({ex.x-1, ex.y-1, ex.w+2, ex.h+2})) { ok = false; break; }
        if (ok) { rooms.push_back(r); for (int ry=y; ry<y+h; ++ry) for (int rx=x; rx<x+w; ++rx) map[ry][rx].type = FLOOR; }
    }
    for (size_t i = 1; i < rooms.size(); ++i) {
        Vec2 p1 = rooms[i-1].center(), p2 = rooms[i].center();
        int xDir = (p2.x > p1.x) ? 1 : -1; for (int x=(int)p1.x; x!=(int)p2.x; x+=xDir) map[(int)p1.y][x].type = FLOOR;
        int yDir = (p2.y > p1.y) ? 1 : -1; for (int y=(int)p1.y; y!=(int)p2.y; y+=yDir) map[y][(int)p2.x].type = FLOOR;
    }
    for (int y = 1; y < MAP_HEIGHT-1; ++y) {
        for (int x = 1; x < MAP_WIDTH-1; ++x) {
            if (map[y][x].type == WALL) {
                bool nearFloor = (map[y+1][x].type == FLOOR || map[y-1][x].type == FLOOR || map[y][x+1].type == FLOOR || map[y][x-1].type == FLOOR);
                if (nearFloor && rand()%12 == 0) {
                    bool horiz = (map[y][x+1].type == WALL || map[y][x-1].type == WALL);
                    decos.push_back(new WallPipe({(float)x*TILE_SIZE+15, (float)y*TILE_SIZE+15}, horiz));
                }
            } else if (map[y][x].type == FLOOR && rand()%60 == 0) {
                decos.push_back(new FloorCable({(float)x*TILE_SIZE+10, (float)y*TILE_SIZE+10}, {(float)x*TILE_SIZE+30, (float)y*TILE_SIZE+30}));
            }
        }
    }
}

void Game::handleInput() {
    SDL_Event e; while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
        if (e.type == SDL_KEYDOWN) {
            keys[e.key.keysym.scancode] = true;
            if (e.key.keysym.scancode == SDL_SCANCODE_SPACE && state == GameState::PLAYING) p->reflexActive = !p->reflexActive;
            if (e.key.keysym.scancode == SDL_SCANCODE_R && state == GameState::PLAYING) {
                int n = p->maxSlugs - p->slugs; if (n > 0 && p->reserveSlugs > 0) { int a = std::min(n, p->reserveSlugs); p->slugs += a; p->reserveSlugs -= a; shake = 2.0f; hud.addLog("RELOADING..."); }
            }
            if (e.key.keysym.scancode == SDL_SCANCODE_LSHIFT && state == GameState::PLAYING && p->energy > 30.0f) {
                Vec2 dir = p->vel.normalized(); if (dir.length() < 0.1f) dir = {0,-1};
                p->vel = dir * DASH_SPEED; p->dashTimer = 0.15f; p->energy -= 30.0f; shake = 10.0f;
            }
            if (e.key.keysym.scancode == SDL_SCANCODE_RETURN && state != GameState::PLAYING) init();
        }
        if (e.type == SDL_KEYUP) keys[e.key.keysym.scancode] = false;
        if (e.type == SDL_MOUSEBUTTONDOWN) mDown = true; 
        if (e.type == SDL_MOUSEBUTTONUP) mDown = false;
        if (e.type == SDL_MOUSEMOTION) { mPos.x = (float)e.motion.x; mPos.y = (float)e.motion.y; }
    }
}

void Game::update() {
    if (state != GameState::PLAYING) return;
    float dt = FRAME_DELAY/1000.0f; if (hitstop > 0) { hitstop -= dt; return; }
    float ts = (p->reflexActive)?REFLEX_SCALE:1.0f; float wdt = dt * ts;
    if (shake > 0) { shake -= 20.0f * dt; if (shake < 0) shake = 0; }
    if (p->dashTimer <= 0) {
        Vec2 mv = {0,0}; if (keys[SDL_SCANCODE_W]) mv.y=-1; if (keys[SDL_SCANCODE_S]) mv.y=1; if (keys[SDL_SCANCODE_A]) mv.x=-1; if (keys[SDL_SCANCODE_D]) mv.x=1;
        p->vel = mv.normalized() * PLAYER_SPEED;
    }
    p->update(dt, map); hud.update(dt);
    for (auto i : items) if (i->active && p->bounds.intersects(i->bounds)) {
        i->active = false; Item* it = (Item*)i;
        switch(it->it) {
            case ItemType::REPAIR_KIT: p->suitIntegrity = std::min(100.0f, p->suitIntegrity+30.0f); hud.addLog("SYSTEM: Integrity restored."); break;
            case ItemType::BATTERY_PACK: p->reserveSlugs += 24; hud.addLog("SYSTEM: Ammunition restocked."); break;
            case ItemType::COOLANT: p->reflexMeter = 100.0f; hud.addLog("SYSTEM: Reflex systems cooled."); break;
            case ItemType::OVERCLOCK: overclocked = true; hud.addLog("CRITICAL: WEAPON OVERCLOCK ACTIVE", {255, 100, 0, 255}); break;
        }
        hitstop = 0.03f;
    }
    items.erase(std::remove_if(items.begin(), items.end(), [](Entity* i){ if(!i->active){ delete i; return true; } return false; }), items.end());
    for (auto c : cores) if (c->active && c->contained && !c->sanitized && p->bounds.intersects(c->bounds)) { 
        c->sanitized = true; score += 150; spawnParts(c->pos, 25, {100, 150, 255, 255});
        hud.addLog("CLEARED: AI Core neutralized.", {100, 255, 255, 255}); hitstop = 0.08f; shake = 15.0f;
    }
    if (mDown && p->shootCooldown <= 0 && p->slugs > 0) {
        Vec2 dir = (mPos + cam - p->bounds.center()).normalized();
        slugs.push_back(new KineticSlug(p->bounds.center(), dir * 800.0f, true));
        p->shootCooldown = overclocked?0.1f:0.25f; p->slugs--; shake = 4.0f;
    }
    bool all = true; for (auto c : cores) if (!c->sanitized) { all = false; break; }
    if (all) exit->active = true;
    if (exit->active && p->bounds.intersects(exit->bounds)) { 
        sector++; 
        saveProgress({sector, score, p->suitIntegrity});
        init(); 
        return; 
    }
    Vec2 tCam = p->bounds.center() - Vec2(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
    cam.x += (tCam.x - cam.x) * 6.0f * dt; cam.y += (tCam.y - cam.y) * 6.0f * dt;
    if (shake >= 1.0f) { cam.x += (rand() % (int)shake) - (int)shake/2; cam.y += (rand() % (int)shake) - (int)shake/2; }
    updateAI(wdt); updateSlugs(wdt); updateEchoes(wdt); updateHazards(wdt); updateParts(wdt); updateLight();
    for (auto& ft : fTexts) { ft.pos.y -= 40.0f * dt; ft.life -= dt; }
    fTexts.erase(std::remove_if(fTexts.begin(), fTexts.end(), [](const FloatingText& t) { return t.life <= 0; }), fTexts.end());
    if (p->suitIntegrity <= 0) state = GameState::GAME_OVER;
}

void Game::updateAI(float dt) {
    for (auto c : cores) {
        if (!c->active || c->contained || c->sanitized) continue;
        float d = c->bounds.center().distance(p->bounds.center());
        if (d < 400) {
            c->stateTimer -= dt; if (c->stateTimer <= 0) { c->calculatePath(p->bounds.center(), map); c->stateTimer = 0.5f; }
            if (!c->path.empty() && c->pathIndex < c->path.size()) {
                Vec2 dir = (c->path[c->pathIndex] - c->bounds.center());
                if (dir.length() < 10.0f) c->pathIndex++; else c->vel = dir.normalized() * AI_SPEED;
            }
        }
        if (d < 250 && rand()%100 < 2) slugs.push_back(new KineticSlug(c->bounds.center(), (p->bounds.center()-c->bounds.center()).normalized()*450.0f, false));
        c->update(dt, map);
    }
}

void Game::updateSlugs(float dt) {
    for (auto s : slugs) {
        if (!s->active) continue; s->update(dt, map);
        if (s->isPlayer) {
            for (auto c : cores) if (c->active && !c->contained && s->bounds.intersects(c->bounds)) {
                GuardianCore* g = dynamic_cast<GuardianCore*>(c);
                if (g && g->shield > 0) { g->shield -= 25.0f * s->powerMultiplier; s->active = false; spawnParts(s->pos, 5, {100, 200, 255, 255}); }
                else { c->stability -= 25.0f * s->powerMultiplier; s->active = false; spawnParts(s->pos, 8, COL_SLUG); if (c->stability <= 0) { c->contained = true; c->vel = {0,0}; score += 50; } }
            }
        } else if (s->bounds.intersects(p->bounds)) { p->suitIntegrity -= 10.0f; s->active = false; spawnParts(s->pos, 5, COL_PLAYER); shake = 8.0f; }
    }
    slugs.erase(std::remove_if(slugs.begin(), slugs.end(), [](KineticSlug* s) { if(!s->active) { delete s; return true; } return false; }), slugs.end());
}

void Game::updateEchoes(float dt) {
    if (state == GameState::PLAYING && rand()%1000 < 1 + sector) echoes.push_back(new NeuralEcho(p->pos + Vec2((float)(rand()%400-200), (float)(rand()%400-200))));
    for (auto e : echoes) {
        e->vel = (p->pos - e->pos).normalized() * 100.0f; e->update(dt, map);
        if (e->active && e->bounds.intersects(p->bounds)) { p->suitIntegrity -= 15.0f; e->active = false; shake = 20.0f; hud.addLog("SYSTEM GLITCH DETECTED!"); }
    }
    echoes.erase(std::remove_if(echoes.begin(), echoes.end(), [](NeuralEcho* e) { if(!e->active) { delete e; return true; } return false; }), echoes.end());
}

void Game::updateHazards(float dt) {
    for (auto h : hazards) {
        h->update(dt, map); ExplosiveCell* cell = (ExplosiveCell*)h;
        if (h->active && !cell->triggered) {
            if (p->bounds.intersects(h->bounds)) cell->onTrigger(*this);
            for(auto s : slugs) if(s->bounds.intersects(h->bounds)) cell->onTrigger(*this);
        }
        if (cell->triggered && cell->fuse <= 0 && h->active) {
            h->active = false; shake = 30.0f; hitstop = 0.1f; spawnParts(h->pos, 40, {255, 200, 50, 255});
            if (p->pos.distance(h->pos) < 100.0f) p->suitIntegrity -= 40.0f;
            for(auto c : cores) if(c->pos.distance(h->pos) < 120.0f) c->stability -= 100.0f;
        }
    }
    hazards.erase(std::remove_if(hazards.begin(), hazards.end(), [](Hazard* h){ if(!h->active){ delete h; return true; } return false; }), hazards.end());
}

void Game::updateParts(float dt) { for (auto& pr : parts) { pr.pos.x += pr.vel.x * dt; pr.pos.y += pr.vel.y * dt; pr.life -= dt; } parts.erase(std::remove_if(parts.begin(), parts.end(), [](const Particle& pr) { return pr.life <= 0; }), parts.end()); }
void Game::spawnParts(Vec2 p, int n, SDL_Color c) { for (int i=0; i<n; ++i) { float a = (float)(rand()%360)*0.0174f, s = 40.0f+(rand()%120); parts.push_back({p, {std::cos(a)*s, std::sin(a)*s}, 0.4f, 0.4f, c, 2.0f+(rand()%2)}); } }

void Game::updateLight() {
    for(auto& r : lMap) std::fill(r.begin(), r.end(), 0.1f);
    Vec2 cp = p->bounds.center(); for (int i=0; i<360; i+=3) {
        float a = (float)i*0.0174f, c = std::cos(a), s = std::sin(a), d = 0;
        while (d < 450.0f) { d += 20.0f; int tx = (int)((cp.x+c*d)/TILE_SIZE), ty = (int)((cp.y+s*d)/TILE_SIZE);
            if (tx>=0 && tx<MAP_WIDTH && ty>=0 && ty<MAP_HEIGHT) { float v = 1.0f-(d/450.0f); if (v>lMap[ty][tx]) lMap[ty][tx]=v; if (map[ty][tx].type==WALL) break; } else break;
        }
    }
}

void Game::render() {
    SDL_SetRenderDrawColor(ren, COL_BG.r, COL_BG.g, COL_BG.b, 255); SDL_RenderClear(ren);
    if (state == GameState::MENU) {
        renderT("RECOIL PROTOCOL", SCREEN_WIDTH/2-180, 180, fontL, {100, 255, 100, 255});
        hud.renderText(ren, "Initialize Security Sweep", SCREEN_WIDTH/2-110, 280, font, COL_TEXT);
        hud.renderText(ren, "Press ENTER to Start", SCREEN_WIDTH/2-80, 350, font, COL_TEXT);
    } else if (state == GameState::PLAYING) {
        int sx = std::max(0, (int)(cam.x/TILE_SIZE)), sy = std::max(0, (int)(cam.y/TILE_SIZE));
        int ex = std::min(MAP_WIDTH, (int)((cam.x+SCREEN_WIDTH)/TILE_SIZE)+1), ey = std::min(MAP_HEIGHT, (int)((cam.y+SCREEN_HEIGHT)/TILE_SIZE)+1);
        for (int y=sy; y<ey; ++y) for (int x=sx; x<ex; ++x) {
            SDL_Rect r = {(int)(x*TILE_SIZE-cam.x), (int)(y*TILE_SIZE-cam.y), TILE_SIZE, TILE_SIZE};
            SDL_SetRenderDrawColor(ren, (map[y][x].type==WALL)?COL_WALL.r:COL_FLOOR.r, (map[y][x].type==WALL)?COL_WALL.g:COL_FLOOR.g, (map[y][x].type==WALL)?COL_WALL.b:COL_FLOOR.b, 255);
            SDL_RenderFillRect(ren, &r); if (map[y][x].type==WALL) { SDL_SetRenderDrawColor(ren, 50, 50, 100, 255); SDL_RenderDrawRect(ren, &r); }
        }
        for (auto d : decos) d->render(ren, cam); for (auto i : items) if(i->active) i->render(ren, cam);
        for (auto h : hazards) if(h->active) h->render(ren, cam);
        if (exit->active) { SDL_SetRenderDrawColor(ren, 100, 255, 100, (Uint8)(150+std::sin(SDL_GetTicks()*0.01f)*100)); SDL_Rect er = {(int)(exit->pos.x-cam.x), (int)(exit->pos.y-cam.y), 40, 40}; SDL_RenderFillRect(ren, &er); }
        for (auto c : cores) c->render(ren, cam); for (auto e : echoes) e->render(ren, cam); p->render(ren, cam);
        for (auto s : slugs) s->render(ren, cam);
        for (auto& pr : parts) { SDL_SetRenderDrawColor(ren, pr.color.r, pr.color.g, pr.color.b, (Uint8)(255*(pr.life/pr.maxLife))); SDL_Rect r = {(int)(pr.pos.x-cam.x), (int)(pr.pos.y-cam.y), (int)pr.size, (int)pr.size}; SDL_RenderFillRect(ren, &r); }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        for (int y=sy; y<ey; ++y) for (int x=sx; x<ex; ++x) {
            int a = (int)(255.0f * (1.0f - lMap[y][x])); if (a>0) { SDL_SetRenderDrawColor(ren, 0, 0, 5, (Uint8)a); SDL_Rect r = {(int)(x*TILE_SIZE-cam.x), (int)(y*TILE_SIZE-cam.y), TILE_SIZE, TILE_SIZE}; SDL_RenderFillRect(ren, &r); }
        }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        hud.render(ren, p, score, sector, overclocked, font, fontL, *this);
    } else if (state == GameState::GAME_OVER) {
        renderT("PROTOCOL FAILURE", SCREEN_WIDTH/2-180, 200, fontL, {255, 50, 50, 255});
        hud.renderText(ren, "Cores Sanitized: " + std::to_string(score/150), SCREEN_WIDTH/2-80, 300, font, COL_TEXT);
        hud.renderText(ren, "Press ENTER to Reboot", SCREEN_WIDTH/2-100, 400, font, COL_TEXT);
    }
    SDL_RenderPresent(ren);
}

void Game::renderT(std::string t, int x, int y, TTF_Font* f, SDL_Color c) {
    if (!f || t.empty()) return; 
    SDL_Surface* s = TTF_RenderText_Blended(f, t.c_str(), c);
    if (s) { SDL_Texture* tx = SDL_CreateTextureFromSurface(ren, s); SDL_Rect d = {x, y, s->w, s->h}; SDL_RenderCopy(ren, tx, NULL, &d); SDL_DestroyTexture(tx); SDL_FreeSurface(s); }
}

void Game::loop() { while (running) { Uint32 st = SDL_GetTicks(); handleInput(); update(); render(); Uint32 t = SDL_GetTicks()-st; if (t<FRAME_DELAY) SDL_Delay((Uint32)FRAME_DELAY - t); } }

int main(int a, char** b) { (void)a; (void)b; srand(time(NULL)); Game g; g.loop(); return 0; }