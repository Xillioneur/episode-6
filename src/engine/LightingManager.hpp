#ifndef LIGHTINGMANAGER_HPP
#define LIGHTINGMANAGER_HPP

#include <vector>
#include <algorithm>
#include <SDL2/SDL.h>
#include "../core/Constants.hpp"
#include "../core/Vec2.hpp"
#include "../core/Enums.hpp"

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

#endif