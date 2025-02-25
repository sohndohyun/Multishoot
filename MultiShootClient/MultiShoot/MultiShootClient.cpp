#include "MultiShootClient.h"
#include <iostream>

MultiShootClient::MultiShootClient() {
}

MultiShootClient::~MultiShootClient() {
	PacketType* packet;
	while (dataQ.pop(&packet)) {
		GameController::DeletePacket(packet);
	}
}

void MultiShootClient::OnUpdate() {

}

void MultiShootClient::OnConnected() {
}

void MultiShootClient::OnSend(int size) {
}

void MultiShootClient::OnRecv(char* data, int size) {
	std::cout << "recv " << size << std::endl;
	auto packet = GameController::CreatePacket(data);
	dataQ.push(packet);
}

void MultiShootClient::OnDisconnected() {
}
