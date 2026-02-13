#include "Actor.hpp"
#include "../core/Constants.hpp"
#include <queue>

namespace Graphics {
    void drawWeapon(SDL_Renderer* ren, Vec2 center, float lookAngle, int length, int width, SDL_Color col, float handOffset) {
        float handAngle = lookAngle + 1.5708f;
        Vec2 handPos = center + Vec2(std::cos(handAngle) * handOffset, std::sin(handAngle) * handOffset);
        float c = std::cos(lookAngle), s = std::sin(lookAngle);
        int ex = (int)(handPos.x + c * length), ey = (int)(handPos.y + s * length);
        SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, 255);
        for(int i = -width/2; i <= width/2; ++i) {
            float ox = -s * i, oy = c * i;
            SDL_RenderDrawLine(ren, (int)(handPos.x + ox), (int)(handPos.y + oy), (int)(ex + ox), (int)(ey + oy));
        }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 255, 50, 50, 80);
        SDL_RenderDrawLine(ren, (int)handPos.x, (int)handPos.y, (int)(handPos.x + c * 150.0f), (int)(handPos.y + s * 150.0f));
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }

    void drawContainment(SDL_Renderer* ren, SDL_Rect r) {
        float pulse = std::abs(std::sin(SDL_GetTicks() * 0.01f));
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 100, 200, 255, (Uint8)(100 + pulse * 100));
        SDL_RenderDrawRect(ren, &r);
        SDL_Rect inner = {r.x + 4, r.y + 4, r.w - 8, r.h - 8};
        SDL_RenderDrawRect(ren, &inner);
        for(int i=0; i<r.w; i+=8) SDL_RenderDrawLine(ren, r.x+i, r.y, r.x+i, r.y+r.h);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }
}

// Player
Player::Player(Vec2 p) : Entity(p, 24, 24, EntityType::PLAYER) {}
void Player::update(float dt, const std::vector<std::vector<Tile>>& map) {
    if (dashTimer > 0) dashTimer -= dt;
    Entity::update(dt, map);
    if (shootCooldown > 0) shootCooldown -= dt;
    energy = std::min(100.0f, energy + 12.0f * dt);
    prevShield = shield;
    if (shield < maxShield) shield = std::min(maxShield, shield + 5.0f * dt);
    if (reflexActive) {
        reflexMeter -= 25.0f * dt;
        if (reflexMeter <= 0) { reflexMeter = 0; reflexActive = false; }
    } else reflexMeter = std::min(100.0f, reflexMeter + 15.0f * dt);
}
void Player::render(SDL_Renderer* ren, const Vec2& cam) {
    SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
    SDL_SetRenderDrawColor(ren, 40, 45, 55, 255); SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, 20, 20, 25, 255); SDL_RenderDrawRect(ren, &r);
    SDL_Rect chest = {r.x + 4, r.y + 4, 16, 16}; SDL_SetRenderDrawColor(ren, 60, 70, 80, 255); SDL_RenderFillRect(ren, &chest);
    SDL_Rect visor = {r.x + 6, r.y + 2, 12, 4}; SDL_SetRenderDrawColor(ren, 100, 255, 255, 255); SDL_RenderFillRect(ren, &visor);
    Graphics::drawWeapon(ren, { (float)r.x + 12, (float)r.y + 12 }, lookAngle, 22, 6, {100, 110, 120, 255}, 0.0f);
    if(dashTimer > 0) { SDL_SetRenderDrawColor(ren, 255, 255, 255, 150); SDL_RenderDrawRect(ren, &r); }
}

// RogueCore
RogueCore::RogueCore(Vec2 p) : Entity(p, 28, 28, EntityType::ROGUE_CORE) {}
RogueCore::RogueCore(Vec2 p, float w, float h, EntityType t) : Entity(p, w, h, t) {}
void RogueCore::render(SDL_Renderer* ren, const Vec2& cam) {
    if (!active) return;
    SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
    if (sanitized) { SDL_SetRenderDrawColor(ren, 40, 45, 55, 255); SDL_RenderFillRect(ren, &r); return; }
    SDL_SetRenderDrawColor(ren, 30, 30, 40, 255); SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, COL_CORE.r, COL_CORE.g, COL_CORE.b, 255); SDL_RenderDrawRect(ren, &r);
    SDL_Rect core = {r.x + 8, r.y + 8, 12, 12}; SDL_RenderFillRect(ren, &core);
    Graphics::drawWeapon(ren, { (float)r.x + 14, (float)r.y + 14 }, lookAngle, 18, 5, {80, 40, 40, 255}, 0.0f);
    if (contained) Graphics::drawContainment(ren, r);
}
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
            if(vis[cy][cx]) continue; 
            vis[cy][cx]=true; 
            int dx[]={0,0,1,-1}, dy[]={1,-1,0,0};
            for(int i=0; i<4; ++i){ 
                int nx=cx+dx[i], ny=cy+dy[i]; 
                if(nx>=0&&nx<MAP_WIDTH&&ny>=0&&ny<MAP_HEIGHT&&map[ny][nx].type!=WALL&&!vis[ny][nx]){ 
                    float tg=gS[cy][cx]+1.0f; 
                    if(tg<gS[ny][nx]){
                        par[ny][nx]={cx,cy}; gS[ny][nx]=tg; pq.push({tg+(float)std::abs(nx-ex)+(float)std::abs(ny-ey), {nx,ny}}); 
                    } 
                } 
            }
        }
    path.clear(); if(found){ int cx=ex, cy=ey; while(cx!=sx||cy!=sy){ path.push_back({(float)cx*TILE_SIZE+20, (float)cy*TILE_SIZE+20}); auto p=par[cy][cx]; cx=p.first; cy=p.second; } std::reverse(path.begin(), path.end()); pathIndex=0; }
}

// Guardian
GuardianCore::GuardianCore(Vec2 p) : RogueCore(p, 52, 52, EntityType::ROGUE_CORE) { stability = 500.0f; }
void GuardianCore::render(SDL_Renderer* ren, const Vec2& cam) {
    if (!active) return;
    if (sanitized) { RogueCore::render(ren, cam); return; }
    SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
    SDL_SetRenderDrawColor(ren, 70, 75, 90, 255); SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, 120, 130, 150, 255); for(int i=0; i<3; ++i) { SDL_Rect plate = {r.x + i*4, r.y + i*4, r.w - i*8, r.h - i*8}; SDL_RenderDrawRect(ren, &plate); }
    SDL_Rect emitter = {r.x + 20, r.y + 20, 12, 12}; SDL_SetRenderDrawColor(ren, 100, 200, 255, 255); SDL_RenderFillRect(ren, &emitter);
    Graphics::drawWeapon(ren, { (float)r.x + 26, (float)r.y + 26 }, lookAngle, 26, 8, {100, 100, 120, 255}, 0.0f);
    if (shield > 0) { SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND); SDL_SetRenderDrawColor(ren, 100, 200, 255, 80); SDL_Rect sr = {r.x - 8, r.y - 8, r.w + 16, r.h + 16}; SDL_RenderDrawRect(ren, &sr); SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE); }
    if (contained) Graphics::drawContainment(ren, r);
}

// Seeker
SeekerSwarm::SeekerSwarm(Vec2 p) : RogueCore(p, 20, 20, EntityType::ROGUE_CORE) { stability = 30.0f; angleOffset = (float)(rand() % 360); }
void SeekerSwarm::update(float dt, const std::vector<std::vector<Tile>>& map) {
    if (stunTimer <= 0) { angleOffset += 5.0f * dt; Vec2 orbit = {std::cos(angleOffset) * 40.0f, std::sin(angleOffset) * 40.0f}; move(orbit * dt, map); }
    RogueCore::update(dt, map);
}
void SeekerSwarm::render(SDL_Renderer* ren, const Vec2& cam) {
    if (!active || sanitized) return;
    SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
    SDL_SetRenderDrawColor(ren, 255, 150, 0, 255); SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255); SDL_RenderDrawLine(ren, r.x, r.y, r.x+r.w, r.y+r.h); SDL_RenderDrawLine(ren, r.x+r.w, r.y, r.x, r.y+r.h);
    Graphics::drawWeapon(ren, { (float)r.x + 10, (float)r.y + 10 }, lookAngle, 14, 3, {200, 100, 0, 255}, 0.0f);
    if (contained) Graphics::drawContainment(ren, r);
}

// Repair
RepairDrone::RepairDrone(Vec2 p) : RogueCore(p, 24, 24, EntityType::ROGUE_CORE) {}
void RepairDrone::render(SDL_Renderer* ren, const Vec2& cam) {
    if (!active || sanitized) return;
    SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
    SDL_SetRenderDrawColor(ren, 40, 80, 40, 255); SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, 100, 255, 100, 255); SDL_RenderDrawRect(ren, &r);
    Graphics::drawWeapon(ren, { (float)r.x + 12, (float)r.y + 12 }, lookAngle, 16, 4, {0, 255, 100, 255}, 0.0f);
    if (contained) Graphics::drawContainment(ren, r);
}

// Boss
FinalBossCore::FinalBossCore(Vec2 p) : RogueCore(p, 96, 96, EntityType::ROGUE_CORE) { stability = 2500.0f; }
void FinalBossCore::render(SDL_Renderer* ren, const Vec2& cam) {
    if (!active) return;
    if (sanitized) { RogueCore::render(ren, cam); return; }
    SDL_Rect r = {(int)(pos.x - cam.x), (int)(pos.y - cam.y), (int)bounds.w, (int)bounds.h};
    SDL_SetRenderDrawColor(ren, 15, 15, 20, 255); SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, 200, 0, 50, 255); for(int i=0; i<6; ++i) { SDL_Rect rim = {r.x + i*3, r.y + i*3, r.w - i*6, r.h - i*6}; SDL_RenderDrawRect(ren, &rim); }
    float pulse = std::abs(std::sin(SDL_GetTicks() * 0.005f)) * 40.0f;
    SDL_Rect eye = {r.x + 36, r.y + 36, 24, 24}; SDL_SetRenderDrawColor(ren, 255, 50, (Uint8)(255 - pulse), 255); SDL_RenderFillRect(ren, &eye);
    Graphics::drawWeapon(ren, { (float)r.x + 48, (float)r.y + 48 }, lookAngle, 48, 12, {150, 50, 50, 255}, 0.0f);
    Graphics::drawWeapon(ren, { (float)r.x + 48, (float)r.y + 48 }, lookAngle + 0.8f, 38, 8, {120, 40, 40, 255}, 0.0f);
    Graphics::drawWeapon(ren, { (float)r.x + 48, (float)r.y + 48 }, lookAngle - 0.8f, 38, 8, {120, 40, 40, 255}, 0.0f);
    if (phase == 2) { SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND); SDL_SetRenderDrawColor(ren, 255, 50, 255, 120); SDL_Rect sr = {r.x - 16, r.y - 16, r.w + 32, r.h + 32}; SDL_RenderDrawRect(ren, &sr); SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE); }
    if (contained) Graphics::drawContainment(ren, r);
}