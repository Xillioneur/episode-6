#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <SDL2/SDL.h>

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

const SDL_Color COL_BG = {5, 5, 10, 255};
const SDL_Color COL_WALL = {35, 40, 55, 255};
const SDL_Color COL_FLOOR = {15, 15, 20, 255};
const SDL_Color COL_PLAYER = {50, 255, 150, 255}; 
const SDL_Color COL_CORE = {255, 80, 50, 255};    
const SDL_Color COL_SLUG = {255, 255, 255, 255};
const SDL_Color COL_ROGUE_SLUG = {255, 150, 50, 255};
const SDL_Color COL_GLITCH = {200, 50, 255, 180};
const SDL_Color COL_TEXT = {220, 230, 255, 255};
const SDL_Color COL_GOLD = {255, 215, 0, 255};
const SDL_Color COL_EMP = {100, 150, 255, 255};
const SDL_Color COL_CONTAINED = {100, 200, 255, 255};

#endif