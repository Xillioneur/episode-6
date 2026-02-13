#ifndef ENUMS_HPP
#define ENUMS_HPP

#include "Rect.hpp"
#include <SDL2/SDL.h>
#include <string>
#include <cstdio>

enum class GameState { MENU, PLAYING, GAME_OVER, VICTORY, SUMMARY };
enum class EntityType { PLAYER, ROGUE_CORE, NEURAL_ECHO, KINETIC_SLUG, ITEM, EXIT, HAZARD, DECORATION, GADGET };
enum class AmmoType { STANDARD, EMP, PIERCING };
enum class ItemType { REPAIR_KIT, BATTERY_PACK, COOLANT, OVERCLOCK };
enum TileType { WALL, FLOOR, HAZARD_TILE };

struct Tile {
    TileType type;
    Rect rect;
};

struct SaveData {
    int sector; int score; float integrity;
};

inline void saveProgress(const SaveData& data) {
    FILE* f = fopen("recoil_save.bin", "wb");
    if (f) { fwrite(&data, sizeof(SaveData), 1, f); fclose(f); }
}

struct Particle {
    Vec2 pos; Vec2 vel; float life; float maxLife; SDL_Color color; float size;
};

struct FloatingText {
    Vec2 pos; std::string text; float life; SDL_Color color;
};

#endif
