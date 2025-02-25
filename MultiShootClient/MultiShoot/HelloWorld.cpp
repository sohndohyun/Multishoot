#include "HelloWorld.hpp"
#include "SingleController.h"
#include "MultiController.h"
#include <iostream>

using namespace std;
using namespace Tvdr;

HelloWorld::HelloWorld(GameMode mode){
	if (mode == GameMode::SINGLE)
		_controller = new SingleController;
	else
		_controller = new MultiController;
	if (_controller->Work()) {
		Start();
		AddChild(_controller);
	}
	else
		GameManager::ChangeScene(new LobbyScene);
}

void HelloWorld::Start(){
	auto background = new GameObject("background.bmp");
	background->SetScale(1.5f, 1.5f);
	AddChild(background);

	_killCount = 0;
	_killCountText = new Text("Plaguard-ZVnjx.ttf", 40);

	AddChild(_killCountText);
	_killCountText->SetLocalPosition(20, 20);

	_bulletCooltime = 0.2f;
	_bulletCoolCounter = 0;

	_id = -1;
	_lastDir = Vector(0, 0);
}

void HelloWorld::Update(){
	while (auto packet = _controller->Pop()) {
		switch (*packet){
		case PacketType::LOGIN_RES:
		{
			std::cout << "login res" << std::endl;
			LoginRes* res = (LoginRes*)packet;
			_id = res->playerID;
			delete res;
		}
			break;
		case PacketType::PLAYER_SPAWN_RES:
		{
			PlayerSpawnRes* res = (PlayerSpawnRes*)packet;
			SpawnPlayer(res->playerID, res->pos, res->hp, res->dir);
			delete res;
		}
			break;
		case PacketType::CHANGE_DIR_RES:
		{
			ChangeDirRes* res = (ChangeDirRes*)packet;
			auto p = FindPlayer(res->playerID);
			if (p) {
				p->SetPosition(res->pos);
				p->SetDir(res->dir);
			}
			delete res;
		}
			break;
		case PacketType::MONSTER_SPAWN_RES:
		{
			MonsterSpawnRes* res = (MonsterSpawnRes*)packet;
			SpawnEnemy(res->monsterID, res->pos, res->hp);
			delete res;
		}
			break;
		case PacketType::SHOOT_RES:
		{
			ShootRes* res = (ShootRes*)packet;
			SpawnBullet(res->bulletID, res->pos);
			delete res;
		}
			break;
		case PacketType::PLAYER_HIT_RES:
		{
			PlayerHitRes* res = (PlayerHitRes*)packet;
			auto p = FindPlayer(res->playerID);
			if (p) {
				p->SetHP(res->playerHP);
				auto m = FindEnemy(res->monsterID);
				if (m)
					m->SetActive(false);
				if (p->GetHP() <= 0)
					p->SetActive(false);
			}
			delete res;
		}
			break;
		case PacketType::MONSTER_HIT_RES:
		{
			MonsterHitRes* res = (MonsterHitRes*)packet;
			auto m = FindEnemy(res->monsterID);
			if (m) {
				if (m->SetHP(res->monsterHP)) {
					m->SetActive(false);
					_killCountText->SetText(to_string(++_killCount));
				}
					
				auto b = FindBullet(res->bulletID);
				if (b) b->SetActive(false);
			}
			delete res;
		}
			break;
		case PacketType::GAME_END_RES:
		{
			std::cout << "game ended" << std::endl;
			GameEndRes* res = (GameEndRes*)packet;
			PlayerPref::SetInt("bestscore", res->bestscore);
			PlayerPref::SetInt("score", res->score);
			GameManager::ChangeScene(new LobbyScene);
			delete res;
		}
			break;
		default:
			break;
		}
	}

	if (_id != -1) {
		if (Input::GetKey(SDL_SCANCODE_SPACE) && _bulletCoolCounter > _bulletCooltime) {
			_controller->Shoot(_id);
			_bulletCoolCounter = 0;
		}

		Vector dir;
		if (Input::GetKey(SDL_SCANCODE_UP))
			dir.y = -1;
		if (Input::GetKey(SDL_SCANCODE_DOWN))
			dir.y = 1;
		if (Input::GetKey(SDL_SCANCODE_LEFT))
			dir.x = -1;
		if (Input::GetKey(SDL_SCANCODE_RIGHT))
			dir.x = 1;
		dir.Norm();

		if (dir != _lastDir){
			_lastDir = dir;
			_controller->ChangeDir(dir, _id);
		}
	}
	_bulletCoolCounter += GameManager::GetDeltaTime();
}

Bullet* HelloWorld::FindNotActiveBullet(){
	for (auto b : _bulletList)
		if (!b->GetActive())
			return b;
	return nullptr;
}

Enemy* HelloWorld::FindNotActiveEnemy(){
	for (auto e : _enemyList)
		if (!e->GetActive())
			return e;
	return nullptr;
}

void HelloWorld::SpawnEnemy(Uint32 id, Vector pos, Uint32 hp){
	auto enemy = FindNotActiveEnemy();
	if (!enemy){
		enemy = new Enemy(id, pos, hp);
		AddChild(enemy);
		_enemyList.push_back(enemy);
	} 
	else {
		enemy->SetActive(true);
		enemy->Init(id, pos, hp);
	}
}

void HelloWorld::SpawnBullet(Uint32 id, Vector pos) {
	auto bullet = FindNotActiveBullet();
	if (!bullet) {
		bullet = new Bullet(id, pos);
		AddChild(bullet);
		_bulletList.push_back(bullet);
	}
	else {
		bullet->SetActive(true);
		bullet->Init(id, pos);
	}
}

void HelloWorld::SpawnPlayer(Uint32 id, Vector pos, Uint32 hp, Vector dir) {
	auto player = new Player(id, pos, hp);
	player->SetDir(dir);
	AddChild(player);
	_playerList.push_back(player);
}

Player* HelloWorld::FindPlayer(Uint32 id) {
	for (auto o : _playerList) {
		if (o->id == id)
			return o;
	}
	return nullptr;
}

Bullet* HelloWorld::FindBullet(Uint32 id) {
	for (auto o : _bulletList) {
		if (o->id == id)
			return o;
	}
	return nullptr;
}

Enemy* HelloWorld::FindEnemy(Uint32 id) {
	for (auto o : _enemyList) {
		if (o->id == id)
			return o;
	}
	return nullptr;
}
