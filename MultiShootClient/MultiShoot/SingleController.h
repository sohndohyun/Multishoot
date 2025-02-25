#pragma once
#include "TVDR.hpp"
#include "GameController.h"
#include <list>

struct SCObject {
	Uint32 id;
	Tvdr::Vector dir;
	Tvdr::Vector size;
	Tvdr::Vector pos;
	Uint32 speed;
	Uint32 hp;
	SCObject() : id(0), speed(0), hp(0) {}
};

class SingleController : public GameController
{
private:

	SCObject _player;
	std::list<SCObject*> _enemy;
	std::list<SCObject*> _bullet;

	float _enemySponTime;
	float _enemySponCounter;

	Uint32 _enemyIDCounter;
	Uint32 _bulletIDCounter;

	Uint32 _score;
	Uint32 _bestscore;

	void SpawnEnemy();
	void UpdatePosition();
	bool PlayerEnemyCollision(SCObject &enemy);
	bool BulletEnemyCollision(SCObject &enemy);
protected:
	virtual void Update();

public:
	SingleController();
	virtual ~SingleController();

	virtual void ChangeDir(Tvdr::Vector dir, Uint32 id);
	virtual void Shoot(Uint32 id);
};

