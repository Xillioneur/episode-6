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
    SDL_Texture* glowTex = nullptr;
    SDL_Texture* shadowMask = nullptr;

    LightingManager() {
        lMap.resize(MAP_HEIGHT);
        for (auto& r : lMap) r.assign(MAP_WIDTH, 0.0f);
    }

    ~LightingManager() {
        if (glowTex) SDL_DestroyTexture(glowTex);
        if (shadowMask) SDL_DestroyTexture(shadowMask);
    }

    void init(SDL_Renderer* ren) {
        if (glowTex) return;
        
        // High-res radial light texture
        int sz = 256;
        SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, sz, sz, 32, SDL_PIXELFORMAT_RGBA32);
        Uint32* pixels = (Uint32*)s->pixels;
        for (int y = 0; y < sz; ++y) {
            for (int x = 0; x < sz; ++x) {
                float dx = (float)x - sz / 2.0f, dy = (float)y - sz / 2.0f;
                float dist = std::sqrt(dx * dx + dy * dy);
                float maxDist = sz / 2.0f;
                float t = std::max(0.0f, 1.0f - (dist / maxDist));
                float alpha = std::pow(t, 3.0f); // Natural falloff
                pixels[y * sz + x] = SDL_MapRGBA(s->format, 255, 255, 255, (Uint8)(alpha * 255));
            }
        }
        glowTex = SDL_CreateTextureFromSurface(ren, s);
        SDL_FreeSurface(s);
        SDL_SetTextureBlendMode(glowTex, SDL_BLENDMODE_ADD);

        // Shadow mask texture for smoothed interpolation
        shadowMask = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, MAP_WIDTH, MAP_HEIGHT);
        SDL_SetTextureBlendMode(shadowMask, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(shadowMask, SDL_ScaleModeLinear);
        SDL_SetTextureScaleMode(glowTex, SDL_ScaleModeLinear);
    }

    void update(const Vec2& cp, const std::vector<std::vector<Tile>>& map) {
        for (auto& r : lMap) std::fill(r.begin(), r.end(), 0.08f); // Ambient floor
        for (int i = 0; i < 360; i += 2) {
            float a = (float)i * 0.0174f;
            float c = std::cos(a), s = std::sin(a), d = 0;
            while (d < 500.0f) {
                d += 10.0f;
                int tx = (int)((cp.x + c * d) / TILE_SIZE), ty = (int)((cp.y + s * d) / TILE_SIZE);
                if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT) {
                    float v = 1.0f - (d / 500.0f);
                    if (v > lMap[ty][tx]) lMap[ty][tx] = v;
                    if (map[ty][tx].type == WALL) break;
                } else break;
            }
        }
    }

    void render(SDL_Renderer* ren, const Vec2& cam) {
        // Update shadow mask texture
        Uint32* pixels;
        int pitch;
        if (SDL_LockTexture(shadowMask, NULL, (void**)&pixels, &pitch) < 0) return;
        
        // Define colors locally to avoid format issues
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                float v = std::clamp(lMap[y][x], 0.0f, 1.0f);
                Uint8 alpha = (Uint8)(235 * (1.0f - v));
                // Hardcode RGBA32 format: 0xAABBGGRR or 0xRRGGBBAA depending on endianness
                // But SDL_LockTexture with PIXELFORMAT_RGBA32 is usually R,G,B,A bytes
                Uint8 r=2, g=2, b=8;
                pixels[y * (pitch / 4) + x] = (alpha << 24) | (b << 16) | (g << 8) | r;
            }
        }
        SDL_UnlockTexture(shadowMask);

        // Render smoothed shadow mask
        SDL_Rect src = { 0, 0, MAP_WIDTH, MAP_HEIGHT };
        SDL_Rect dst = { (int)-cam.x, (int)-cam.y, MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE };
        SDL_RenderCopy(ren, shadowMask, &src, &dst);
    }

    void drawPointLight(SDL_Renderer* ren, Vec2 p, float rad, SDL_Color c, float intensity) {
        if (!glowTex) return;
        SDL_SetTextureColorMod(glowTex, c.r, c.g, c.b);
        SDL_SetTextureAlphaMod(glowTex, (Uint8)intensity);
        SDL_Rect r = { (int)(p.x - rad), (int)(p.y - rad), (int)rad * 2, (int)rad * 2 };
        SDL_RenderCopy(ren, glowTex, NULL, &r);
    }
};

#endif