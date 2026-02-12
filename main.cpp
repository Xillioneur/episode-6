#include "src/Game.hpp"
#include <ctime>
#include <SDL2/SDL.h>

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    srand((unsigned int)time(NULL));
    {
        Game game;
        game.loop();
    }
    SDL_Quit();
    return 0;
}
