#pragma once

#include "TVDR.hpp"
#include <queue>

enum class PacketType {
	CHANGE_DIR_REQ, SHOOT_REQ,

	LOGIN_RES,
	PLAYER_SPAWN_RES,
	CHANGE_DIR_RES,
	SHOOT_RES,
	MONSTER_SPAWN_RES,
	MONSTER_HIT_RES,
	PLAYER_HIT_RES,
	GAME_END_RES
};

struct ChangeDirReq {
	PacketType type;
	Tvdr::Vector dir;
	Uint32 playerID;
};

struct ShootReq {
	PacketType type;
	Uint32 playerID;
};

struct LoginRes {
	PacketType type;
	Uint32 playerID;
};

struct PlayerSpawnRes {
	PacketType type;
	Uint32 playerID;
	Tvdr::Vector dir;
	Tvdr::Vector pos;
	Uint32 hp;
};


struct ChangeDirRes {
	PacketType type;
	Tvdr::Vector dir;
	Tvdr::Vector pos;
	Uint32 playerID;
};

struct ShootRes {
	PacketType type;
	Tvdr::Vector pos;
	Uint32 bulletID;
};

struct MonsterSpawnRes {
	PacketType type;
	Tvdr::Vector pos;
	Uint32 monsterID;
	Uint32 hp;
};

struct MonsterHitRes {
	PacketType type;
	Uint32 monsterID;
	Uint32 bulletID;
	Uint32 monsterHP;
};

struct PlayerHitRes {
	PacketType type;
	Uint32 playerID;
	Uint32 monsterID;
	Uint32 playerHP;
};

struct GameEndRes {
	PacketType type;
	Uint32 playerID;
	Uint32 score;
	Uint32 bestscore;
};

class GameController : public Tvdr::Object
{
protected:
	std::queue<PacketType*> _resQ;
public:
	virtual void ChangeDir(Tvdr::Vector dir, Uint32 id) = 0;
	virtual void Shoot(Uint32 id) = 0;
	virtual bool Work() { return true; }

	PacketType* Pop();
	static void DeletePacket(PacketType* pt);
	static PacketType* CreatePacket(char* data);

	virtual ~GameController();
};

