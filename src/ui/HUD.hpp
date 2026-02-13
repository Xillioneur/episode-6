#ifndef HUD_HPP
#define HUD_HPP

#include <vector>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../gameplay/Actor.hpp"

class Game;

class HUD {
public:
    struct LogEntry { std::string msg; float life; SDL_Color col; };
    std::vector<LogEntry> logs;

    void addLog(const std::string& m, SDL_Color c = {200, 200, 255, 255});
    void update(float dt);
    void render(SDL_Renderer* ren, Player* p, int score, int sector, Game& game, TTF_Font* font, TTF_Font* fontL);
    void renderSummary(SDL_Renderer* ren, int score, int sector, TTF_Font* font, TTF_Font* fontL);
    void renderText(SDL_Renderer* ren, const std::string& t, int x, int y, TTF_Font* f, SDL_Color c);

private:
    void drawBar(SDL_Renderer* ren, int x, int y, int w, int h, float pct, SDL_Color col);
};

#endif
