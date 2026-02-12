#include "src/Game.hpp"
#include <ctime>

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    srand((unsigned int)time(NULL));
    Game game;
    game.loop();
    return 0;
}
