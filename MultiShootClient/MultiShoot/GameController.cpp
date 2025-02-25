#include "GameController.h"

GameController::~GameController() {
	while (!_resQ.empty()) {
		auto packet = _resQ.front();
		_resQ.pop();
		DeletePacket(packet);
	}
}

PacketType* GameController::Pop() {
	if (_resQ.empty())
		return nullptr;
	auto temp = _resQ.front();
	_resQ.pop();
	return temp;
}

void GameController::DeletePacket(PacketType* packet) {
	switch (*packet) {
	case PacketType::CHANGE_DIR_REQ:
	{
		ChangeDirReq* req = (ChangeDirReq*)packet;
		delete req;
		break;
	}
	case PacketType::SHOOT_REQ:
	{
		ShootReq* req = (ShootReq*)packet;
		delete req;
		break;
	}
	case PacketType::LOGIN_RES:
	{
		LoginRes* res = (LoginRes*)packet;
		delete res;
	}
	break;
	case PacketType::PLAYER_SPAWN_RES:
	{
		PlayerSpawnRes* res = (PlayerSpawnRes*)packet;
		delete res;
	}
	break;
	case PacketType::CHANGE_DIR_RES:
	{
		ChangeDirRes* res = (ChangeDirRes*)packet;
		delete res;
	}
	break;
	case PacketType::MONSTER_SPAWN_RES:
	{
		MonsterSpawnRes* res = (MonsterSpawnRes*)packet;
		delete res;
	}
	break;
	case PacketType::SHOOT_RES:
	{
		ShootRes* res = (ShootRes*)packet;
		delete res;
	}
	break;
	case PacketType::PLAYER_HIT_RES:
	{
		PlayerHitRes* res = (PlayerHitRes*)packet;
		delete res;
	}
	break;
	case PacketType::MONSTER_HIT_RES:
	{
		MonsterHitRes* res = (MonsterHitRes*)packet;
		delete res;
	}
	break;
	case PacketType::GAME_END_RES:
	{
		GameEndRes* res = (GameEndRes*)packet;
		delete res;
	}
	break;
	}
}

PacketType* GameController::CreatePacket(char* data) {
	PacketType* packet = (PacketType*)data;
	switch (*packet) {
	case PacketType::LOGIN_RES:
	{
		LoginRes* res = (LoginRes*)data;
		auto rt = new LoginRes;
		rt->playerID = res->playerID;
		rt->type = PacketType::LOGIN_RES;
		return (PacketType*)rt;
	}
	case PacketType::PLAYER_SPAWN_RES:
	{
		PlayerSpawnRes* res = (PlayerSpawnRes*)packet;
		return (PacketType*)(new PlayerSpawnRes(*res));
	}
	case PacketType::CHANGE_DIR_RES:
	{
		ChangeDirRes* res = (ChangeDirRes*)packet;
		return (PacketType*)(new ChangeDirRes(*res));
	}
	case PacketType::MONSTER_SPAWN_RES:
	{
		MonsterSpawnRes* res = (MonsterSpawnRes*)packet;
		return (PacketType*)(new MonsterSpawnRes(*res));
	}
	case PacketType::SHOOT_RES:
	{
		ShootRes* res = (ShootRes*)packet;
		return (PacketType*)(new ShootRes(*res));
	}
	case PacketType::PLAYER_HIT_RES:
	{
		PlayerHitRes* res = (PlayerHitRes*)packet;
		return (PacketType*)(new PlayerHitRes(*res));
	}
	case PacketType::MONSTER_HIT_RES:
	{
		MonsterHitRes* res = (MonsterHitRes*)packet;
		return (PacketType*)(new MonsterHitRes(*res));
	}
	case PacketType::GAME_END_RES:
	{
		GameEndRes* res = (GameEndRes*)packet;
		return (PacketType*)(new GameEndRes(*res));
	}
	}
	return nullptr;
}