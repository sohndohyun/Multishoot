#include "LobbyScene.hpp"
#include "TVDR.hpp"

int main(int argc, char* args[]) {
	Tvdr::GameManager::Init({ 600, 800 }, "Terminvader");
	return Tvdr::GameManager::Run(new LobbyScene);
}