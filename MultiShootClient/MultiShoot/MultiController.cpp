#include "MultiController.h"
#include <iostream>
using namespace Tvdr;
using namespace std;

MultiController::MultiController() {
	_tool.init("127.0.0.1", 3000);
	_work = _tool.start();
}

MultiController::~MultiController() {
	if (_work) {
		_tool.end();
	}
}

void MultiController::Update() {

	PacketType* packet;
	if (_tool.dataQ.pop(&packet)) {
		if (packet == nullptr)
		{
			cout << "packet nullptr" << endl;
		}
		else
			_resQ.push(packet);
	}
}

void MultiController::ChangeDir(Vector dir, Uint32 id) {
	if (_work) {
		ChangeDirReq req;
		req.dir = dir;
		req.playerID = id;
		req.type = PacketType::CHANGE_DIR_REQ;
		_tool.send_data((char*)&req, sizeof(ChangeDirReq));
	}
}

void MultiController::Shoot(Uint32 id) {
	if (_work) {
		ShootReq req;
		req.playerID = id;
		req.type = PacketType::SHOOT_REQ;
		_tool.send_data((char*)&req, sizeof(ShootReq));
	}
}