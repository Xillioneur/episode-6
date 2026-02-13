#include "HUD.hpp"
#include "../Game.hpp"
#include <algorithm>

void HUD::addLog(const std::string& m, SDL_Color c) {
    logs.push_back({m, 5.0f, c});
    if (logs.size() > 6) logs.erase(logs.begin());
}

void HUD::update(float dt) {
    for (auto& l : logs) l.life -= dt;
    logs.erase(std::remove_if(logs.begin(), logs.end(), [](const LogEntry& le) { return le.life <= 0; }), logs.end());
}

void HUD::renderText(SDL_Renderer* ren, const std::string& t, int x, int y, TTF_Font* f, SDL_Color c) {
    if (!f || t.empty()) return;
    SDL_Surface* s = TTF_RenderText_Blended(f, t.c_str(), c);
    if (s) {
        SDL_Texture* tx = SDL_CreateTextureFromSurface(ren, s);
        SDL_Rect dst = {x, y, s->w, s->h};
        SDL_RenderCopy(ren, tx, NULL, &dst);
        SDL_DestroyTexture(tx);
        SDL_FreeSurface(s);
    }
}

void HUD::drawBar(SDL_Renderer* ren, int x, int y, int w, int h, float pct, SDL_Color col) {
    SDL_Rect bg = {x, y, w, h};
    SDL_SetRenderDrawColor(ren, 40, 40, 40, 255);
    SDL_RenderFillRect(ren, &bg);
    SDL_Rect fg = {x, y, (int)(w * std::clamp(pct, 0.0f, 1.0f)), h};
    SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
    SDL_RenderFillRect(ren, &fg);
    SDL_SetRenderDrawColor(ren, 80, 80, 80, 255);
    SDL_RenderDrawRect(ren, &bg);
}

void HUD::renderSummary(SDL_Renderer* ren, int score, int sector, TTF_Font* font, TTF_Font* fontL) {
    renderText(ren, "SECTOR " + std::to_string(sector) + " CLEARED", SCREEN_WIDTH / 2 - 200, 100, fontL, {50, 150, 255, 255});
    renderText(ren, "STATUS: CORE SANITIZATION COMPLETE", SCREEN_WIDTH / 2 - 140, 170, font, {200, 200, 255, 255});
    
    SDL_SetRenderDrawColor(ren, 50, 60, 80, 255);
    SDL_Rect line = {SCREEN_WIDTH / 2 - 200, 210, 400, 2};
    SDL_RenderFillRect(ren, &line);

    renderText(ren, "FINANCIAL ASSETS RECOVERED: " + std::to_string(score), SCREEN_WIDTH / 2 - 120, 240, font, {255, 255, 255, 255});
    renderText(ren, "SECTOR PERFORMANCE RATING: S-CLASS", SCREEN_WIDTH / 2 - 140, 270, font, {255, 255, 100, 255});
    
    renderText(ren, "MISSION LOG HISTORY:", SCREEN_WIDTH / 2 - 80, 330, font, {150, 150, 150, 255});
    int sy = 360;
    for (const auto& l : logs) {
        renderText(ren, l.msg, SCREEN_WIDTH / 2 - 150, sy, font, {100, 100, 150, 255});
        sy += 20;
    }
    
    SDL_RenderFillRect(ren, &line); line.y = 500; SDL_RenderFillRect(ren, &line);
    renderText(ren, "PRESS ENTER TO PROCEED TO NEXT SECTOR", SCREEN_WIDTH / 2 - 160, 530, font, {50, 255, 100, 255});
    
    SDL_SetRenderDrawColor(ren, 100, 255, 100, (Uint8)(100 + 100 * std::sin(SDL_GetTicks() * 0.01f)));
    SDL_Rect flash = {SCREEN_WIDTH / 2 - 170, 520, 340, 40};
    SDL_RenderDrawRect(ren, &flash);
}

void HUD::render(SDL_Renderer* ren, Player* p, int score, int sector, Game& game, TTF_Font* font, TTF_Font* fontL) {
    (void)fontL;
    drawBar(ren, 20, 20, 200, 18, p->suitIntegrity/100.0f, {50, 255, 100, 255});
    renderText(ren, "INTEGRITY", 25, 21, font, {255, 255, 255, 255});
    drawBar(ren, 20, 45, 150, 10, p->energy/100.0f, {50, 150, 255, 255});
    renderText(ren, "ENERGY", 25, 45, font, {200, 200, 255, 255});
    drawBar(ren, 20, 60, 150, 10, p->reflexMeter / 100.0f, {255, 200, 50, 255});
    renderText(ren, "REFLEX", 25, 60, font, {255, 255, 200, 255});
    
    renderText(ren, "SLUGS: " + std::to_string(p->slugs) + " / " + std::to_string(p->reserveSlugs), 20, 85, font, {220, 220, 220, 255});
    std::string ammoStr = (game.currentAmmo == AmmoType::STANDARD) ? "STANDARD" : (game.currentAmmo == AmmoType::EMP) ? "EMP" : "PIERCING";
    renderText(ren, "AMMO: " + ammoStr, 20, 100, font, {150, 255, 255, 255});
    
    if(game.debugMode) renderText(ren, "DEBUG ACTIVE", SCREEN_WIDTH-150, 20, font, {255, 255, 0, 255});
    renderText(ren, "SECTOR: " + std::to_string(sector), SCREEN_WIDTH-120, 40, font, {150, 150, 255, 255});
    renderText(ren, "SCORE: " + std::to_string(score), SCREEN_WIDTH-120, 60, font, {255, 255, 255, 255});
    renderText(ren, game.objective.getDesc(), SCREEN_WIDTH/2-150, 20, font, {255, 255, 100, 255});
    
    // Minimap
    SDL_Rect mmRect = {SCREEN_WIDTH - 110, 100, 100, 100}; SDL_SetRenderDrawColor(ren, 0, 0, 0, 180); SDL_RenderFillRect(ren, &mmRect);
    float mapScale = 0.04f; Vec2 mapCtr = {(float)mmRect.x + 50, (float)mmRect.y + 50};
    for(auto c : game.cores) {
        if(!c->sanitized) {
            Vec2 rel = (c->pos - p->pos) * mapScale;
            if(std::abs(rel.x)<48 && std::abs(rel.y)<48) {
                SDL_Rect d = {(int)(mapCtr.x+rel.x-1), (int)(mapCtr.y+rel.y-1), 3, 3};
                SDL_SetRenderDrawColor(ren, 255, 50, 50, 255); SDL_RenderFillRect(ren, &d);
            }
        }
    }
    if(game.exit && game.exit->active) {
        Vec2 rel = (game.exit->pos - p->pos) * mapScale;
        if(std::abs(rel.x)<48 && std::abs(rel.y)<48) {
            SDL_Rect d = {(int)(mapCtr.x+rel.x-2), (int)(mapCtr.y+rel.y-2), 5, 5};
            SDL_SetRenderDrawColor(ren, 100, 255, 100, 255); SDL_RenderFillRect(ren, &d);
        }
    }
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255); SDL_Rect pDot = {(int)mapCtr.x-2, (int)mapCtr.y-2, 4, 4}; SDL_RenderFillRect(ren, &pDot);

    // Directional
    if (game.exit && game.exit->active) {
        Vec2 dir = (game.exit->pos - p->pos).normalized();
        Vec2 arrowPos = { SCREEN_WIDTH / 2.0f + dir.x * 65.0f, SCREEN_HEIGHT / 2.0f + dir.y * 65.0f };
        SDL_Rect arrow = {(int)arrowPos.x - 4, (int)arrowPos.y - 4, 8, 8};
        SDL_SetRenderDrawColor(ren, 100, 255, 100, 255); SDL_RenderFillRect(ren, &arrow);
    }

    int logY = SCREEN_HEIGHT - 120; for (auto& l : logs) { renderText(ren, "> " + l.msg, 20, logY, font, l.col); logY += 18; }
}
