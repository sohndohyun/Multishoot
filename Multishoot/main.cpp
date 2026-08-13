#include "LobbyScene.hpp"
#include "TVDR.hpp"

int main(int argc, char* args[]) {
    tvdr::game_manager::initialize({600, 800}, "Terminvader");
    return tvdr::game_manager::run(new lobby_scene);
}