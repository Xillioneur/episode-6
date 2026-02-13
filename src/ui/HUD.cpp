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
    
    // Bloom shadow
    SDL_Color bloom = c; bloom.a = 60;
    SDL_Surface* bs = TTF_RenderText_Blended(f, t.c_str(), bloom);
    if (bs) {
        SDL_Texture* bt = SDL_CreateTextureFromSurface(ren, bs);
        SDL_Rect bd = {x - 1, y - 1, bs->w + 2, bs->h + 2};
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(ren, bt, NULL, &bd);
        SDL_DestroyTexture(bt); SDL_FreeSurface(bs);
    }

    SDL_Surface* s = TTF_RenderText_Blended(f, t.c_str(), c);
    if (s) {
        SDL_Texture* tx = SDL_CreateTextureFromSurface(ren, s);
        SDL_Rect dst = {x, y, s->w, s->h};
        SDL_RenderCopy(ren, tx, NULL, &dst);
        SDL_DestroyTexture(tx);
        SDL_FreeSurface(s);
    }
}

void HUD::renderMenu(SDL_Renderer* ren, TTF_Font* font, TTF_Font* fontL) {
    renderText(ren, "RECOIL PROTOCOL", SCREEN_WIDTH / 2 - 180, 150, fontL, {100, 200, 255, 255});
    renderText(ren, "NEURAL INTERFACE INITIALIZED", SCREEN_WIDTH / 2 - 120, 220, font, {200, 200, 255, 255});
    
    int ty = 280;
    renderText(ren, "OPERATIONAL MANUAL:", SCREEN_WIDTH / 2 - 80, ty, font, {150, 150, 150, 255});
    renderText(ren, "[WASD] - MOBILE LINK", SCREEN_WIDTH / 2 - 100, ty + 25, font, {200, 200, 200, 255});
    renderText(ren, "[MOUSE1] - KINETIC DISCHARGE", SCREEN_WIDTH / 2 - 100, ty + 45, font, {200, 200, 200, 255});
    renderText(ren, "[SHIFT] - TACHYON DASH", SCREEN_WIDTH / 2 - 100, ty + 65, font, {200, 200, 200, 255});
    renderText(ren, "[SPACE] - REFLEX OVERRIDE", SCREEN_WIDTH / 2 - 100, ty + 85, font, {200, 200, 200, 255});
    renderText(ren, "[1-3] - AMMO SELECTION", SCREEN_WIDTH / 2 - 100, ty + 105, font, {200, 200, 200, 255});
    
    renderText(ren, "PRESS ENTER TO COMMENCE MISSION", SCREEN_WIDTH / 2 - 160, 480, font, {100, 255, 100, 255});
    renderText(ren, "CREATED BY GEMINI-CLI // SYSTEM VERSION 1.0.3", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT - 30, font, {80, 80, 100, 255});
}

void HUD::drawBar(SDL_Renderer* ren, int x, int y, int w, int h, float pct, SDL_Color col) {
    SDL_Rect bg = {x, y, w, h};
    SDL_SetRenderDrawColor(ren, 20, 20, 25, 255);
    SDL_RenderFillRect(ren, &bg);
    
    SDL_Rect fg = {x, y, (int)(w * std::clamp(pct, 0.0f, 1.0f)), h};
    SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, 255);
    SDL_RenderFillRect(ren, &fg);
    
    // Glass highlight
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 40);
    SDL_Rect hi = { x, y, w, h / 2 };
    SDL_RenderFillRect(ren, &hi);
    
    SDL_SetRenderDrawColor(ren, 60, 60, 70, 255);
    SDL_RenderDrawRect(ren, &bg);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
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
    drawBar(ren, 20, 40, 200, 6, p->shield / p->maxShield, {100, 200, 255, 255});
    drawBar(ren, 20, 50, 150, 10, p->energy/100.0f, {50, 150, 255, 255});
    renderText(ren, "ENERGY", 25, 50, font, {200, 200, 255, 255});
    drawBar(ren, 20, 65, 150, 10, p->reflexMeter / 100.0f, {255, 200, 50, 255});
    renderText(ren, "REFLEX", 25, 65, font, {255, 255, 200, 255});
    
    renderText(ren, "SLUGS: " + std::to_string(p->slugs) + " / " + std::to_string(p->reserveSlugs), 20, 85, font, {220, 220, 220, 255});
    std::string ammoStr = (game.currentAmmo == AmmoType::STANDARD) ? "STANDARD" : (game.currentAmmo == AmmoType::EMP) ? "EMP" : "PIERCING";
    renderText(ren, "AMMO: " + ammoStr, 20, 100, font, {150, 255, 255, 255});
    
    renderText(ren, "SECTOR: " + std::to_string(sector), SCREEN_WIDTH-120, 40, font, {150, 150, 255, 255});
    renderText(ren, "SCORE: " + std::to_string(score), SCREEN_WIDTH-120, 60, font, {255, 255, 255, 255});
    
    if (game.multiplier > 1.0f) {
        std::string mStr = "MULT: x" + std::to_string(game.multiplier).substr(0, 3);
        renderText(ren, mStr, SCREEN_WIDTH - 120, 80, font, {255, 200, 50, (Uint8)(150 + 105 * (game.multiplierTimer / 3.0f))});
    }

    renderText(ren, game.objective.getDesc(), SCREEN_WIDTH/2-150, 20, font, {255, 255, 100, 255});

    for (auto c : game.cores) {
        if (dynamic_cast<FinalBossCore*>(c) && !c->sanitized) {
            FinalBossCore* bc = dynamic_cast<FinalBossCore*>(c);
            drawBar(ren, SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 40, 400, 12, bc->stability / 2500.0f, {255, 50, 50, 255});
            renderText(ren, "BOSS ANOMALY STABILITY", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT - 38, font, {255, 255, 255, 200});
            break;
        }
    }
    
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
