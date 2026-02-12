#ifndef GAME_HPP
#define GAME_HPP

#include <vector>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "core/Constants.hpp"
#include "core/Enums.hpp"
#include "core/Vec2.hpp"
#include "engine/InputHandler.hpp"
#include "engine/LightingManager.hpp"
#include "engine/VFXManager.hpp"
#include "ui/HUD.hpp"
#include "gameplay/Actor.hpp"
#include "gameplay/Slug.hpp"
#include "gameplay/Item.hpp"

class ObjectiveSystem {
public:
    enum Type { CLEAR_CORES, REACH_EXIT };
    Type currentType = CLEAR_CORES;
    void update(Game& game);
    std::string getDesc() const;
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

#endif
