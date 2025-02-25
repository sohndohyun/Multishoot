#include "SingleController.h"

using namespace Tvdr;

SingleController::SingleController() {
	_player.id = 0;
	_player.dir = Vector(0, 0);
	_player.hp = 5;
	_player.size = Vector(79, 54);
	_player.pos = Vector(260, 500);
	_player.speed = 150;

	_enemySponCounter = 4;
	_enemySponTime = 3;

	_enemyIDCounter = 0;
	_bulletIDCounter = 0;

	_score = 0;
	_bestscore = PlayerPref::GetInt("bestscore");

	auto login = new LoginRes;
	login->playerID = 0;
	login->type = PacketType::LOGIN_RES;
	_resQ.push((PacketType*)login);

	auto pspon = new PlayerSpawnRes;
	pspon->playerID = _player.id;
	pspon->pos = Vector(_player.pos);
	pspon->hp = _player.hp;
	pspon->dir = Vector(0, 0);
	pspon->type = PacketType::PLAYER_SPAWN_RES;
	_resQ.push((PacketType*)pspon);
}

SingleController::~SingleController() {
	for (auto it = _bullet.begin(); it != _bullet.end();) {
		delete (*it);
		it = _bullet.erase(it);
	}

	for (auto it = _enemy.begin(); it != _enemy.end();) {
		delete (*it);
		it = _enemy.erase(it);
	}
}

void SingleController::SpawnEnemy() {
	for (int i = 0; i < 5; ++i) {
		auto enemy = new SCObject;
		enemy->id = _enemyIDCounter++;
		enemy->dir = Vector(0, 1);
		enemy->hp = 5;
		enemy->size = Vector(43, 51);
		enemy->pos = Vector(120.f * i + 60.f - 21.5f, 0);
		enemy->speed = 200;
		_enemy.push_back(enemy);

		auto res = new MonsterSpawnRes;
		res->monsterID = enemy->id;
		res->pos = enemy->pos;
		res->hp = enemy->hp;
		res->type = PacketType::MONSTER_SPAWN_RES;
		_resQ.push((PacketType*)res);
	}
}

void SingleController::UpdatePosition() {
	auto dt = GameManager::GetDeltaTime();
	_player.pos = _player.pos + _player.dir * _player.speed * dt;

	for (auto it = _bullet.begin(); it != _bullet.end();) {
		(*it)->pos = (*it)->pos + (*it)->dir * (*it)->speed * dt;
		if ((*it)->pos.y < -25)
			it = _bullet.erase(it);
		else {
			++it;
		}
	}

	for (auto it = _enemy.begin(); it != _enemy.end();) {
		auto enemy = *it;
		enemy->pos = enemy->pos + enemy->dir * enemy->speed * dt;
		if (enemy->pos.y > 800 || BulletEnemyCollision(*enemy) || PlayerEnemyCollision(*enemy)) {
			it = _enemy.erase(it);
			delete enemy;
		}
		else
			++it;
	}
}

bool SingleController::PlayerEnemyCollision(SCObject& enemy) {
	Rect pr(_player.pos, _player.size);

	Rect er(enemy.pos, enemy.size);
	if (Rect::IsOverlapped(pr, er)) {
		auto res = new PlayerHitRes;
		res->monsterID = enemy.id;
		res->playerHP = --_player.hp;
		res->playerID = _player.id;
		res->type = PacketType::PLAYER_HIT_RES;
		_resQ.push((PacketType*)res);

		if (_player.hp <= 0){
			auto res = new GameEndRes;
			res->bestscore = _bestscore < _score ? _score : _bestscore;
			res->score = _score;
			res->type = PacketType::GAME_END_RES;
			_resQ.push((PacketType*)res);
		}

		return true;
	}
	return false;
}

bool SingleController::BulletEnemyCollision(SCObject& enemy) {
	Rect er(enemy.pos, enemy.size);
	for (auto it = _bullet.begin(); it != _bullet.end();) {
		Rect br((*it)->pos, (*it)->size);
		if (Rect::IsOverlapped(er, br)) {
			auto res = new MonsterHitRes;
			res->bulletID = (*it)->id;
			res->monsterHP = --enemy.hp;
			res->monsterID = enemy.id;
			res->type = PacketType::MONSTER_HIT_RES;
			_resQ.push((PacketType*)res);

			auto bullet = *it;
			_bullet.erase(it);
			delete bullet;

			if (enemy.hp > 0) 
				return false;
			++_score;
			return true;
		}
		else
			++it;
	}
	return false;
}

void SingleController::Update() {
	if (_enemySponCounter >= _enemySponTime){
		this->SpawnEnemy();
		_enemySponCounter = 0;
	}
	this->UpdatePosition();

	_enemySponCounter += GameManager::GetDeltaTime();
}

void SingleController::ChangeDir(Tvdr::Vector dir, Uint32 id) {
	_player.dir = dir;

	auto res = new ChangeDirRes;
	res->dir = dir;
	res->playerID = _player.id;
	res->pos = _player.pos;
	res->type = PacketType::CHANGE_DIR_RES;
	_resQ.push((PacketType*)res);
}

void SingleController::Shoot(Uint32 id) {
	auto bullet = new SCObject;
	bullet->id = _bulletIDCounter++;
	bullet->dir = Vector(0, -1);
	bullet->size = Vector(24, 8);
	bullet->pos = _player.pos + Vector(28, -4);
	bullet->speed = 300;
	bullet->hp = 0;
	_bullet.push_back(bullet);

	auto res = new ShootRes;
	res->type = PacketType::SHOOT_RES;
	res->bulletID = bullet->id;
	res->pos = bullet->pos;
	_resQ.push((PacketType*)res);
}
