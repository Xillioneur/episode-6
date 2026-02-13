#include "Game.hpp"
#include <iostream>
#include <queue>

void ObjectiveSystem::update(Game& game) {
    bool all = true;
    for (auto c : game.cores) {
        if (!c->sanitized) { all = false; break; }
    }
    if (all) {
        currentType = REACH_EXIT;
        if (game.exit) game.exit->active = true;
    } else {
        currentType = CLEAR_CORES;
    }
}

std::string ObjectiveSystem::getDesc() const {
    return (currentType == CLEAR_CORES) ? "OBJECTIVE: Neutralize Rogue AI Cores." : "OBJECTIVE: Proceed to extraction point.";
}

Game::Game() {
    TTF_Init();
    win = SDL_CreateWindow("Recoil Protocol", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#ifdef __APPLE__
    font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 18); fontL = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 52);
#else
    font = TTF_OpenFont("arial.ttf", 18); fontL = TTF_OpenFont("arial.ttf", 52);
#endif
    init();
}

Game::~Game() {
    cleanup();
    if(font) TTF_CloseFont(font);
    if(fontL) TTF_CloseFont(fontL);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
}

void Game::init() {
    cleanup();
    generateLevel();
    p = new Player(findSpace(24, 24));
    p->reserveSlugs = 60;
    for (int i = 0; i < 5 + sector * 2; ++i) cores.push_back(new RogueCore(findSpace(28, 28)));
    if (sector % 2 == 0) {
        for (int i = 0; i < 2 + sector / 2; ++i) cores.push_back(new SeekerSwarm(findSpace(20, 20)));
    }
    if (sector % 5 == 0) {
        cores.push_back(new FinalBossCore(findSpace(80, 80)));
        hud.addLog("CRITICAL: BOSS ANOMALY DETECTED!", {255, 50, 50, 255});
    }
    for (int i = 0; i < 8; ++i) items.push_back(new Item(findSpace(20, 20), (rand() % 100 < 40) ? ItemType::BATTERY_PACK : ItemType::REPAIR_KIT));
    exit = new Entity(findSpace(40, 40), 40, 40, EntityType::EXIT);
    exit->active = false;
    state = GameState::PLAYING;
    hud.addLog("SYSTEM ONLINE. SECTOR " + std::to_string(sector));
    audio.play(SoundType::POWERUP, 0.4f, 200.0f);
    titleTimer = 3.0f;
}

void Game::cleanup() {
    if(p) delete p;
    for(auto c:cores) delete c;
    for(auto s:slugs) delete s;
    for(auto e:echoes) delete e;
    for(auto i:items) delete i;
    if(exit) delete exit;
    cores.clear(); slugs.clear(); echoes.clear(); items.clear(); fTexts.clear();
}

Vec2 Game::findSpace(float w, float h) {
    for(int i = 0; i < 2000; ++i) {
        int x = 1 + rand() % (MAP_WIDTH - 2);
        int y = 1 + rand() % (MAP_HEIGHT - 2);
        if (map[y][x].type == FLOOR) {
            Rect r = {(float)x * TILE_SIZE + 2, (float)y * TILE_SIZE + 2, w, h};
            bool safe = true;
            for (int sy = y - 1; sy <= y + 2; sy++) {
                for (int sx = x - 1; sx <= x + 2; sx++) {
                    if (sx >= 0 && sx < MAP_WIDTH && sy >= 0 && sy < MAP_HEIGHT && map[sy][sx].type == WALL && r.intersects(map[sy][sx].rect)) safe = false;
                }
            }
            if (safe) return {r.x, r.y};
        }
    }
    return {MAP_WIDTH * TILE_SIZE / 2.0f, MAP_HEIGHT * TILE_SIZE / 2.0f};
}

void Game::generateLevel() {
    bool connected = false;
    while (!connected) {
        map.assign(MAP_HEIGHT, std::vector<Tile>(MAP_WIDTH));
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                map[y][x].type = WALL;
                map[y][x].rect = {(float)x * TILE_SIZE, (float)y * TILE_SIZE, (float)TILE_SIZE, (float)TILE_SIZE};
            }
        }
        std::vector<Rect> rooms;
        for (int i = 0; i < 15; ++i) {
            int w = 6 + rand() % 6, h = 6 + rand() % 6, x = 1 + rand() % (MAP_WIDTH - w - 1), y = 1 + rand() % (MAP_HEIGHT - h - 1);
            Rect r = {(float)x, (float)y, (float)w, (float)h};
            bool ok = true;
            for (const auto& ex : rooms) if (r.intersects({ex.x - 1, ex.y - 1, ex.w + 2, ex.h + 2})) { ok = false; break; }
            if (ok) {
                rooms.push_back(r);
                for (int ry = y; ry < y + h; ++ry) for (int rx = (int)x; rx < (int)x + w; ++rx) map[ry][rx].type = FLOOR;
            }
        }
        for (size_t i = 1; i < rooms.size(); ++i) {
            Vec2 p1 = rooms[i - 1].center(), p2 = rooms[i].center();
            int xDir = (p2.x > p1.x) ? 1 : -1; for (int x = (int)p1.x; x != (int)p2.x; x += xDir) map[(int)p1.y][x].type = FLOOR;
            int yDir = (p2.y > p1.y) ? 1 : -1; for (int y = (int)p1.y; y != (int)p2.y; y += yDir) map[y][(int)p2.x].type = FLOOR;
        }
        if (rooms.empty()) continue;
        std::vector<std::vector<bool>> reachable(MAP_HEIGHT, std::vector<bool>(MAP_WIDTH, false));
        std::queue<std::pair<int, int>> q; Vec2 start = rooms[0].center(); q.push({(int)start.x, (int)start.y}); reachable[(int)start.y][(int)start.x] = true;
        while (!q.empty()) {
            auto cur = q.front(); q.pop();
            int dx[] = {0, 0, 1, -1}, dy[] = {1, -1, 0, 0};
            for (int i = 0; i < 4; ++i) {
                int nx = cur.first + dx[i], ny = cur.second + dy[i];
                if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT && map[ny][nx].type == FLOOR && !reachable[ny][nx]) { reachable[ny][nx] = true; q.push({nx, ny}); }
            }
        }
        connected = true;
        for (int y = 0; y < MAP_HEIGHT; ++y) for (int x = 0; x < MAP_WIDTH; ++x) if (map[y][x].type == FLOOR && !reachable[y][x]) connected = false;
    }
}

void Game::handleInput() {
    input.update();
    if (input.isTriggered(SDL_SCANCODE_F1)) { 
        debugMode = !debugMode; hud.addLog(debugMode ? "DEV MODE: ON" : "DEV MODE: OFF", COL_GOLD); 
        audio.play(SoundType::UI_CLICK, 0.3f, 800.0f);
    }
    if (debugMode && input.isTriggered(SDL_SCANCODE_F2)) { sector++; init(); return; }
    if (input.isTriggered(SDL_SCANCODE_SPACE) && state == GameState::PLAYING) { 
        p->reflexActive = !p->reflexActive; hud.addLog(p->reflexActive ? "REFLEX: ON" : "REFLEX: OFF"); 
        audio.play(SoundType::UI_CONFIRM, 0.4f, 400.0f);
    }
    if (input.isPressed(SDL_SCANCODE_1) && currentAmmo != AmmoType::STANDARD) { currentAmmo = AmmoType::STANDARD; audio.play(SoundType::UI_CLICK, 0.2f, 600.0f); }
    if (input.isPressed(SDL_SCANCODE_2) && currentAmmo != AmmoType::EMP) { currentAmmo = AmmoType::EMP; audio.play(SoundType::UI_CLICK, 0.2f, 700.0f); }
    if (input.isPressed(SDL_SCANCODE_3) && currentAmmo != AmmoType::PIERCING) { currentAmmo = AmmoType::PIERCING; audio.play(SoundType::UI_CLICK, 0.2f, 800.0f); }
    if (input.isTriggered(SDL_SCANCODE_R) && state == GameState::PLAYING) {
        int n = p->maxSlugs - p->slugs;
        if (n > 0 && p->reserveSlugs > 0) {
            int a = std::min(n, p->reserveSlugs); p->slugs += a; p->reserveSlugs -= a; shake = 5.0f; hud.addLog("WEAPON: Slugs reloaded.");
            audio.play(SoundType::RELOAD, 0.3f, 800.0f);
        } else {
            audio.play(SoundType::EMPTY, 0.4f, 150.0f);
        }
    }
    if (input.isPressed(SDL_SCANCODE_F) && p->energy > 50.0f) {
        p->energy -= 50.0f; vfx.triggerFlash(0.5f);
        audio.play(SoundType::POWERUP, 0.4f, 600.0f);
        for (auto s : slugs) if (!s->isPlayer && s->pos.distance(p->pos) < 250.0f) s->active = false;
        for (auto c : cores) if (c->pos.distance(p->pos) < 200.0f) { c->stability -= 150.0f; Vec2 d = (c->pos - p->pos).normalized(); if (d.length() < 0.1f) d = {0, -1}; c->vel = d * 1200.0f; c->stunTimer = 0.8f; }
    }
    if (input.isPressed(SDL_SCANCODE_LSHIFT) && p->energy > 30.0f) {
        Vec2 d = p->vel.normalized(); if (d.length() < 0.1f) d = {0, -1};
        p->vel = d * DASH_SPEED; p->dashTimer = 0.15f; p->energy -= 30.0f;
        audio.play(SoundType::DASH, 0.3f, 200.0f);
    }
    if (input.isPressed(SDL_SCANCODE_RETURN)) {
        if (state == GameState::SUMMARY) {
            sector++; SaveData sd = {sector, score, p->suitIntegrity}; saveProgress(sd);
            audio.play(SoundType::UI_CONFIRM, 0.5f, 400.0f); init();
        } else if (state != GameState::PLAYING) {
            audio.play(SoundType::UI_CONFIRM, 0.5f, 300.0f); init();
        }
    }
}

void Game::update() {
    if (state != GameState::PLAYING) return;
    float dt = FRAME_DELAY / 1000.0f; float ts = p->reflexActive ? REFLEX_SCALE : 1.0f; float wdt = dt * ts;
    if (shake > 0) shake -= 20.0f * dt;
    if (multiplierTimer > 0) multiplierTimer -= dt; else multiplier = 1.0f;
    if (titleTimer > 0) titleTimer -= dt;

    if (p->dashTimer <= 0) {
        Vec2 mv = {0, 0};
        if (input.isPressed(SDL_SCANCODE_W)) mv.y = -1;
        if (input.isPressed(SDL_SCANCODE_S)) mv.y = 1;
        if (input.isPressed(SDL_SCANCODE_A)) mv.x = -1;
        if (input.isPressed(SDL_SCANCODE_D)) mv.x = 1;
        p->vel = mv.normalized() * PLAYER_SPEED;
        if (mv.length() > 0.1f) {
            p->stepTimer -= dt;
            if (p->stepTimer <= 0) { audio.play(SoundType::STEP, 0.15f, 100.0f); p->stepTimer = 0.35f; }
        } else { p->stepTimer = 0; }
    }
    Vec2 pCtr = p->bounds.center(); Vec2 mWorld = input.mPos + cam; p->lookAngle = std::atan2(mWorld.y - pCtr.y, mWorld.x - pCtr.x);
    p->update(dt, map); objective.update(*this); hud.update(dt);
    
    bool bossActive = false;
    bool enemiesClose = false;
    for (auto c : cores) {
        if (dynamic_cast<FinalBossCore*>(c) && !c->sanitized) bossActive = true;
        if (!c->sanitized && c->pos.distance(p->pos) < 350.0f) enemiesClose = true;
    }
    if (bossActive) audio.setAmbientState(AmbientState::BOSS);
    else if (enemiesClose) audio.setAmbientState(AmbientState::BATTLE);
    else audio.setAmbientState(AmbientState::STANDARD);

    if (p->suitIntegrity < 30.0f) {
        alertTimer -= dt;
        if (alertTimer <= 0) { audio.play(SoundType::ALERT, 0.2f, 1000.0f); alertTimer = 0.6f; }
    }
    
    float minCDist = 9999.0f;
    for (auto c : cores) if (!c->sanitized) minCDist = std::min(minCDist, c->pos.distance(p->pos));
    if (minCDist < 250.0f) {
        pulseTimer -= dt;
        if (pulseTimer <= 0) {
            audio.play(SoundType::UI_CLICK, 0.1f, 100.0f + (250.0f - minCDist));
            pulseTimer = 0.4f + (minCDist / 500.0f);
        }
    }

    if (p->energy < 20.0f) {
        energyAlertTimer -= dt;
        if (energyAlertTimer <= 0) { audio.play(SoundType::LOW_ENERGY, 0.15f, 1500.0f); energyAlertTimer = 1.0f; }
    }
    if (debugMode) { p->suitIntegrity = 100.0f; p->energy = 100.0f; p->slugs = p->maxSlugs; }
    updatePickups(); updateWeapons(wdt); updateAI(wdt); updateSlugs(wdt); updateEchoes(wdt);
    for (auto& ft : fTexts) { ft.pos.y -= 40.0f * dt; ft.life -= dt; }
    fTexts.erase(std::remove_if(fTexts.begin(), fTexts.end(), [](const FloatingText& t) { return t.life <= 0; }), fTexts.end());
    if (exit && exit->active && p->bounds.intersects(exit->bounds)) {
        state = GameState::SUMMARY;
        audio.play(SoundType::UI_CONFIRM, 0.6f, 500.0f);
        return;
    }
    Vec2 tCam = p->bounds.center() - Vec2(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2); cam.x += (tCam.x - cam.x) * 6.0f * dt; cam.y += (tCam.y - cam.y) * 6.0f * dt;
    if (shake >= 1.0f) { cam.x += (rand() % (int)shake) - (int)shake / 2; cam.y += (rand() % (int)shake) - (int)shake / 2; }
    lighting.update(p->bounds.center(), map); vfx.update(dt);
    if (p->suitIntegrity <= 0) { 
        state = GameState::GAME_OVER; 
        audio.play(SoundType::BOSS_PHASE, 0.8f, 50.0f); 
        hud.addLog("CRITICAL: SUIT INTEGRITY TERMINATED", {255, 50, 50, 255});
    }
}

void Game::updateWeapons(float dt) {
    (void)dt;
    if (input.mDown && p->shootCooldown <= 0) {
        if (p->slugs > 0) {
            Vec2 d = (input.mPos + cam - p->bounds.center()).normalized();
            slugs.push_back(new KineticSlug(p->bounds.center(), d * 800.0f, true, currentAmmo));
            p->shootCooldown = 0.25f; p->slugs--; shake = 3.0f;
            if (currentAmmo == AmmoType::EMP) audio.play(SoundType::EMP_SHOT, 0.4f, 800.0f);
            else if (currentAmmo == AmmoType::PIERCING) audio.play(SoundType::PIERCE_SHOT, 0.5f, 400.0f);
            else audio.play(SoundType::SHOOT, 0.35f, 1200.0f + (rand() % 200), 0.0f);
        } else {
            p->shootCooldown = 0.25f;
            audio.play(SoundType::EMPTY, 0.3f, 200.0f);
        }
    }
}

void Game::playSpatial(SoundType type, Vec2 pos, float vol, float freq) {
    Vec2 d = pos - p->bounds.center();
    float dist = d.length();
    if (dist > 800.0f) return;
    float pan = std::clamp(d.x / 400.0f, -1.0f, 1.0f);
    float falloff = std::max(0.0f, 1.0f - (dist / 800.0f));
    audio.play(type, vol * falloff, freq, pan);
}

void Game::updatePickups() {
    for (auto i : items) {
        if (i->active && p->bounds.intersects(i->bounds)) {
            i->active = false;
            if (i->it == ItemType::REPAIR_KIT) { 
                p->suitIntegrity = std::min(100.0f, p->suitIntegrity + 30.0f); 
                spawnFText(i->pos, "REPAIRED", {50, 255, 50, 255});
                playSpatial(SoundType::PICKUP, i->pos, 0.4f, 600.0f);
            }
            else if (i->it == ItemType::BATTERY_PACK) { 
                p->reserveSlugs += 24; 
                spawnFText(i->pos, "+24 SLUGS", COL_GOLD);
                playSpatial(SoundType::PICKUP, i->pos, 0.4f, 800.0f);
            }
            else if (i->it == ItemType::COOLANT) {
                p->energy = std::min(100.0f, p->energy + 50.0f);
                spawnFText(i->pos, "ENERGY RESTORED", {100, 100, 255, 255});
                playSpatial(SoundType::POWERUP, i->pos, 0.5f, 1000.0f);
            }
            else if (i->it == ItemType::OVERCLOCK) {
                p->reflexMeter = 100.0f;
                spawnFText(i->pos, "SYSTEM OVERCLOCKED", COL_GOLD);
                playSpatial(SoundType::POWERUP, i->pos, 0.6f, 1200.0f);
            }
            vfx.spawnBurst(i->pos, 15, COL_GOLD);
        }
    }
    items.erase(std::remove_if(items.begin(), items.end(), [](Item* i) { if (!i->active) { delete i; return true; } return false; }), items.end());
}

void Game::updateAI(float dt) {
    std::vector<RogueCore*> newSpawns;
    for (auto c : cores) {
        if (!c->active || c->sanitized) continue;
        Vec2 dirToPlayer = p->bounds.center() - c->bounds.center(); c->lookAngle = std::atan2(dirToPlayer.y, dirToPlayer.x);
        RepairDrone* dr = dynamic_cast<RepairDrone*>(c);
        if (dr) {
            if (!dr->target) {
                float ms = 101.0f;
                for (auto tc : cores) if (tc->active && !tc->sanitized && !tc->contained && tc->stability < ms && tc->type == EntityType::ROGUE_CORE) { ms = tc->stability; dr->target = tc; }
            }
            if (dr->target) {
                Vec2 dir = (dr->target->pos - dr->pos);
                if (dir.length() < 40.0f) { dr->target->stability = std::min(100.0f, dr->target->stability + dr->repairPower * dt); dr->vel = {0, 0}; }
                else dr->vel = dir.normalized() * 180.0f;
            }
            dr->update(dt, map); continue;
        }
        FinalBossCore* bs = dynamic_cast<FinalBossCore*>(c);
        if (bs && !bs->contained) {
            bs->phaseTimer += dt;
            if (bs->phase == 1 && bs->stability < 1000.0f) { 
                bs->phase = 2; hud.addLog("BOSS: Shielding protocol engaged!", {255, 0, 255, 255}); 
                audio.play(SoundType::BOSS_PHASE, 0.7f, 100.0f);
            }
            if (bs->phase == 2 && rand() % 200 == 0) newSpawns.push_back(new SeekerSwarm(bs->bounds.center()));
        }
        if (c->contained) continue;
        if (c->stunTimer > 0) { c->stunTimer -= dt; c->vel = c->vel * std::pow(0.1f, dt); c->update(dt, map); continue; }
        float d = c->bounds.center().distance(p->bounds.center());
        if (d < 400) {
            c->stateTimer -= dt; if (c->stateTimer <= 0) { c->calculatePath(p->bounds.center(), map); c->stateTimer = 0.5f; }
            if (!c->path.empty() && c->pathIndex < c->path.size()) { Vec2 dir = (c->path[c->pathIndex] - c->bounds.center()); if (dir.length() < 10.0f) c->pathIndex++; else c->vel = dir.normalized() * AI_SPEED; }
        }
        if (d < 250 && rand() % 100 < 2) {
            slugs.push_back(new KineticSlug(c->bounds.center(), (p->bounds.center() - c->bounds.center()).normalized() * 450.0f, false));
            playSpatial(SoundType::SHOOT, c->pos, 0.2f, 600.0f + (rand() % 100));
        }
        c->update(dt, map);
    }
    for (auto n : newSpawns) cores.push_back(n);
    cores.erase(std::remove_if(cores.begin(), cores.end(), [](RogueCore* c) { if (!c->active) { delete c; return true; } return false; }), cores.end());
    for (auto c : cores) if (c->contained && !c->sanitized && p->bounds.intersects(c->bounds)) { 
        c->sanitized = true; score += (int)(150 * multiplier); multiplier += 0.2f; multiplierTimer = 3.0f;
        spawnFText(c->pos, "SANITIZED x" + std::to_string(multiplier).substr(0,3), COL_PLAYER); 
        vfx.spawnBurst(c->pos, 25, COL_PLAYER); 
        playSpatial(SoundType::SANITIZE, c->pos, 0.5f, 400.0f);
        audio.play(SoundType::UI_CLICK, 0.3f, 1000.0f + multiplier * 100.0f);
    }
}

void Game::updateSlugs(float dt) {
    for (auto s : slugs) {
        if (!s->active) continue;
        int oldBounces = s->bounces;
        s->update(dt, map);
        if (s->bounces < oldBounces) {
            playSpatial(SoundType::RICOCHET, s->pos, 0.15f, 1200.0f + (rand() % 800));
        }
        
        if (s->isPlayer) {
            for (auto c : cores) if (c->active && !c->contained && s->bounds.intersects(c->bounds)) {
                float dmg = 25.0f * s->powerMultiplier;
                if (s->ammoType == AmmoType::EMP) { c->stunTimer = 1.2f; dmg *= 0.5f; }
                if (s->ammoType == AmmoType::PIERCING) dmg *= 1.5f;
                GuardianCore* g = dynamic_cast<GuardianCore*>(c);
                if (g && g->shield > 0) { 
                    float os = g->shield; g->shield -= dmg; 
                    if (g->shield <= 0) playSpatial(SoundType::SHIELD_DOWN, s->pos, 0.45f, 600.0f);
                    if (s->ammoType != AmmoType::PIERCING) s->active = false; vfx.spawnBurst(s->pos, 5, {100, 200, 255, 255}); playSpatial(SoundType::HIT, s->pos, 0.25f, 800.0f); 
                }
                else { 
                    c->stability -= dmg; s->active = false; vfx.spawnBurst(s->pos, 8, COL_SLUG); 
                    playSpatial(SoundType::HIT, s->pos, 0.3f, 400.0f);
                    if (c->stability <= 0) { c->contained = true; c->vel = {0, 0}; score += (int)(50 * multiplier); multiplier += 0.1f; multiplierTimer = 3.0f; } 
                }
            }
        } else if (s->bounds.intersects(p->bounds)) { 
            p->suitIntegrity -= 10.0f; s->active = false; vfx.spawnBurst(s->pos, 5, COL_PLAYER); shake = 8.0f; 
            audio.play(SoundType::HIT, 0.5f, 200.0f);
            multiplier = 1.0f; multiplierTimer = 0;
        }
    }
    slugs.erase(std::remove_if(slugs.begin(), slugs.end(), [](KineticSlug* s) { if (!s->active) { delete s; return true; } return false; }), slugs.end());
}

void Game::updateEchoes(float dt) {
    if (state == GameState::PLAYING && rand() % 1000 < 1 + sector) echoes.push_back(new NeuralEcho(p->pos + Vec2((float)(rand() % 400 - 200), (float)(rand() % 400 - 200))));
    for (auto e : echoes) { 
        e->vel = (p->pos - e->pos).normalized() * 100.0f; e->update(dt, map); 
        if (e->active && e->bounds.intersects(p->bounds)) { 
            p->suitIntegrity -= 15.0f; e->active = false; vfx.triggerFlash(0.3f); 
            audio.play(SoundType::HIT, 0.6f, 150.0f);
        } 
    }
    echoes.erase(std::remove_if(echoes.begin(), echoes.end(), [](NeuralEcho* e) { if (!e->active) { delete e; return true; } return false; }), echoes.end());
}

void Game::spawnFText(Vec2 pos, std::string t, SDL_Color c) { 
    fTexts.push_back({pos, t, 1.0f, c}); 
    audio.play(SoundType::UI_CLICK, 0.15f, 1500.0f);
}

void Game::render() {
    SDL_SetRenderDrawColor(ren, COL_BG.r, COL_BG.g, COL_BG.b, 255); SDL_RenderClear(ren);
    if (state == GameState::MENU || state == GameState::SUMMARY || state == GameState::GAME_OVER) {
        SDL_SetRenderDrawColor(ren, 20, 30, 40, 255);
        for (int i = 0; i < SCREEN_WIDTH; i += 40) SDL_RenderDrawLine(ren, i, 0, i, SCREEN_HEIGHT);
        for (int i = 0; i < SCREEN_HEIGHT; i += 40) SDL_RenderDrawLine(ren, 0, i, SCREEN_WIDTH, i);
    }
    if (state == GameState::MENU) { 
    renderT("RECOIL PROTOCOL", SCREEN_WIDTH / 2 - 180, 180, fontL, COL_PLAYER); 
    renderT("Press ENTER to Start", SCREEN_WIDTH / 2 - 100, 350, font, COL_TEXT); 
    renderT("CREATED BY GEMINI-CLI AGENT", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT - 40, font, {100, 100, 120, 255});
    }
    else if (state == GameState::PLAYING) {
        int sx = std::max(0, (int)(cam.x / TILE_SIZE)), sy = std::max(0, (int)(cam.y / TILE_SIZE));
        int ex = std::min(MAP_WIDTH, (int)((cam.x + SCREEN_WIDTH) / TILE_SIZE) + 1), ey = std::min(MAP_HEIGHT, (int)((cam.y + SCREEN_HEIGHT) / TILE_SIZE) + 1);
        for (int y = sy; y < ey; ++y) for (int x = sx; x < ex; ++x) {
            SDL_Rect r = {(int)(x * TILE_SIZE - cam.x), (int)(y * TILE_SIZE - cam.y), TILE_SIZE, TILE_SIZE};
            SDL_SetRenderDrawColor(ren, (map[y][x].type == WALL) ? COL_WALL.r : COL_FLOOR.r, (map[y][x].type == WALL) ? COL_WALL.g : COL_FLOOR.g, (map[y][x].type == WALL) ? COL_WALL.b : COL_FLOOR.b, 255); SDL_RenderFillRect(ren, &r);
            if (map[y][x].type == WALL) { SDL_SetRenderDrawColor(ren, 50, 50, 100, 255); SDL_RenderDrawRect(ren, &r); }
        }
        if (exit) {
            SDL_Rect er = {(int)(exit->pos.x - cam.x), (int)(exit->pos.y - cam.y), 40, 40};
            if (exit->active) { SDL_SetRenderDrawColor(ren, 100, 255, 100, (Uint8)(150 + std::sin(SDL_GetTicks() * 0.01f) * 100)); SDL_RenderFillRect(ren, &er); SDL_SetRenderDrawColor(ren, 255, 255, 255, 255); SDL_RenderDrawRect(ren, &er); renderT("EXTRACTION POINT", er.x - 20, er.y - 25, font, {100, 255, 100, 255}); }
            else { SDL_SetRenderDrawColor(ren, 40, 40, 80, 100); SDL_RenderFillRect(ren, &er); SDL_SetRenderDrawColor(ren, 150, 50, 50, 255); SDL_RenderDrawRect(ren, &er); renderT("EXIT LOCKED", er.x - 10, er.y - 25, font, {150, 50, 50, 255}); }
        }
        for (auto c : cores) c->render(ren, cam); p->render(ren, cam); for (auto s : slugs) s->render(ren, cam); for (auto i : items) i->render(ren, cam); for (auto e : echoes) e->render(ren, cam);
        for (const auto& ft : fTexts) { renderT(ft.text, (int)(ft.pos.x - cam.x), (int)(ft.pos.y - cam.y), font, ft.color); }
        vfx.render(ren, cam); lighting.render(ren, cam); hud.render(ren, p, score, sector, *this, font, fontL);
        if (titleTimer > 0) {
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 0, 0, 0, (Uint8)(std::min(1.0f, titleTimer) * 200));
            SDL_Rect tr = {0, SCREEN_HEIGHT / 2 - 60, SCREEN_WIDTH, 120};
            SDL_RenderFillRect(ren, &tr);
            renderT("SECTOR " + std::to_string(sector), SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 40, fontL, COL_PLAYER);
            renderT("OBJECTIVE: " + objective.getDesc(), SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 10, font, COL_TEXT);
        }
    } else if (state == GameState::SUMMARY) {
        hud.renderSummary(ren, score, sector, font, fontL);
    } else { renderT("PROTOCOL FAILURE", SCREEN_WIDTH / 2 - 180, 200, fontL, {255, 50, 50, 255}); renderT("Press ENTER to Reboot", SCREEN_WIDTH / 2 - 100, 400, font, COL_TEXT); }
    SDL_RenderPresent(ren);
}

void Game::renderT(std::string t, int x, int y, TTF_Font* f, SDL_Color c) {
    if (!f || t.empty()) return; SDL_Surface* s = TTF_RenderText_Blended(f, t.c_str(), c);
    if (s) { SDL_Texture* tx = SDL_CreateTextureFromSurface(ren, s); SDL_Rect d = {x, y, s->w, s->h}; SDL_RenderCopy(ren, tx, NULL, &d); SDL_DestroyTexture(tx); SDL_FreeSurface(s); }
}

void Game::loop() { while (running) { Uint32 st = SDL_GetTicks(); handleInput(); update(); render(); Uint32 t = SDL_GetTicks() - st; if (t < FRAME_DELAY) SDL_Delay((Uint32)FRAME_DELAY - t); } }