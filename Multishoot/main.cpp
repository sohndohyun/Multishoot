#include "scenes/lobby_scene.hpp"
#include "engine/game_manager.hpp"

int main(int argc, char* args[]) {
    game_manager::initialize({600, 800}, "Terminvader");
    return game_manager::run(new lobby_scene);
}
