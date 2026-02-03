#include "game.hpp"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Game game;
    if (!game.Init()) {
        return 1;
    }

    game.Run();
    game.Shutdown();
    return 0;
}
