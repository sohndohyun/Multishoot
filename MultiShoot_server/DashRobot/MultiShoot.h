#pragma once
#include "DRServer.h"
#include <string>

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
	uint32_t playerID;
};

struct ShootReq {
	PacketType type;
	uint32_t playerID;
};

struct LoginRes {
	PacketType type;
	uint32_t playerID;
};

struct PlayerSpawnRes {
	PacketType type;
	uint32_t playerID;
	Tvdr::Vector dir;
	Tvdr::Vector pos;
	uint32_t hp;
};


struct ChangeDirRes {
	PacketType type;
	Tvdr::Vector dir;
	Tvdr::Vector pos;
	uint32_t playerID;
};

struct ShootRes {
	PacketType type;
	Tvdr::Vector pos;
	uint32_t bulletID;
};

struct MonsterSpawnRes {
	PacketType type;
	Tvdr::Vector pos;
	uint32_t monsterID;
	uint32_t hp;
};

struct MonsterHitRes {
	PacketType type;
	uint32_t monsterID;
	uint32_t bulletID;
	uint32_t monsterHP;
};

struct PlayerHitRes {
	PacketType type;
	uint32_t playerID;
	uint32_t monsterID;
	uint32_t playerHP;
};

struct GameEndRes {
	PacketType type;
	uint32_t playerID;
	uint32_t score;
	uint32_t bestscore;
};

struct SCObject {
	uint32_t id;
	SOCKET sock;
	Tvdr::Vector dir;
	Tvdr::Vector size;
	Tvdr::Vector pos;
	uint32_t speed;
	uint32_t hp;
	uint32_t score;
};

class MultiShoot : public DRServer
{
public:
	MultiShoot();
	virtual ~MultiShoot();
protected:
	virtual void OnUpdate(float dt);
	virtual void OnAccept(SOCKET sock);
	virtual void OnRecv(SOCKET sock, char* data, int size);
	virtual void OnLeave(SOCKET sock);
	virtual void OnSend(SOCKET sock, int size);
private:
	std::list<SCObject*> _player;
	std::list<SCObject*> _enemy;
	std::list<SCObject*> _bullet;

	float _enemySponTime;
	float _enemySponCounter;

	uint32_t _enemyIDCounter;
	uint32_t _bulletIDCounter;
	uint32_t _playerIDCounter;

	uint32_t _score;

	void SpawnEnemy();
	void UpdatePosition(float dt);
	bool PlayerEnemyCollision(SCObject& enemy);
	bool BulletEnemyCollision(SCObject& enemy);
	SCObject* FindPlayer(uint32_t id);

	void SendToAll(char* data, int size);
	void FirstData(SOCKET sock);

	std::atomic<bool> _connected;
};

  