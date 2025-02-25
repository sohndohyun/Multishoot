#pragma once

#include "TVDR.hpp"
#include "GameController.h"
#include "LobbyScene.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Bullet.hpp"
#include <list>

class HelloWorld : public Tvdr::Scene {
protected:
	void Start();
	virtual void Update();

	Bullet* FindNotActiveBullet();
	Enemy* FindNotActiveEnemy();

	void SpawnEnemy(Uint32 id, Tvdr::Vector pos, Uint32 hp);
	void SpawnBullet(Uint32 id, Tvdr::Vector pos);
	void SpawnPlayer(Uint32 id, Tvdr::Vector pos, Uint32 hp, Tvdr::Vector dir);

	Player* FindPlayer(Uint32 id);
	Bullet* FindBullet(Uint32 id);
	Enemy* FindEnemy(Uint32 id);
public:
	HelloWorld(GameMode mode = GameMode::SINGLE);
	virtual ~HelloWorld(){}

private:
	GameController* _controller;

	std::list<Player*> _playerList;
	std::list<Bullet*> _bulletList;
	std::list<Enemy*> _enemyList;

	Tvdr::Text* _killCountText;

	Uint32 _id;
	Tvdr::Vector _lastDir;
	Uint32 _killCount;

	float _bulletCooltime;
	float _bulletCoolCounter;

};