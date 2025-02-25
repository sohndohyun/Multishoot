#include "stdafx.h"
#include <iostream>
#include "MultiShoot.h"

using namespace Tvdr;

MultiShoot::MultiShoot() {
	_enemySponCounter = 4;
	_enemySponTime = 3;

	_enemyIDCounter = 0;
	_bulletIDCounter = 0;
	_playerIDCounter = 0;

	_score = 0;

	_connected = true;
}

MultiShoot::~MultiShoot() {
	for (auto it = _bullet.begin(); it != _bullet.end();) {
		delete (*it);
		it = _bullet.erase(it);
	}

	for (auto it = _enemy.begin(); it != _enemy.end();) {
		delete (*it);
		it = _enemy.erase(it);
	}

	for (auto it = _player.begin(); it != _player.end();) {
		delete (*it);
		it = _player.erase(it);
	}
	_connected.exchange(false);
}

void MultiShoot::OnSend(SOCKET sock, int size) {
	fprintf(stdout, "[%d] send %d \n", (int)sock, size);
}

void MultiShoot::OnAccept(SOCKET sock)
{
	fprintf(stdout, "[%d] accept \n", (int)sock);
	SCObject* obj = new SCObject;
	obj->id = _playerIDCounter++;
	obj->dir = Vector(0, 0);
	obj->hp = 5;
	obj->size = Vector(79, 54);
	obj->pos = Vector(260, 500);
	obj->speed = 150;
	obj->sock = sock;
	obj->score = 0;

	LoginRes res;
	res.playerID = obj->id;
	res.type = PacketType::LOGIN_RES;
	send_data(sock, (char*)&res, sizeof(res));

	FirstData(sock);
	_player.push_back(obj);

	PlayerSpawnRes pres;
	pres.hp = obj->hp;
	pres.playerID = obj->id;
	pres.pos = obj->pos;
	pres.dir = obj->dir;
	pres.type = PacketType::PLAYER_SPAWN_RES;
	SendToAll((char*)&pres, sizeof(pres));
}

void MultiShoot::OnRecv(SOCKET sock, char* data, int size)
{
	PacketType* packet = (PacketType*)data;
	switch (*packet)
	{
	case PacketType::CHANGE_DIR_REQ:
	{
		ChangeDirReq* req = (ChangeDirReq*)packet;
		auto player = FindPlayer(req->playerID);
		if (player) {
			player->dir = req->dir;

			ChangeDirRes res;
			res.dir = player->dir;
			res.pos = player->pos;
			res.playerID = player->id;
			res.type = PacketType::CHANGE_DIR_RES;
			SendToAll((char*)&res, sizeof(res));
		}
	}
		break;
	case PacketType::SHOOT_REQ:
	{
		ShootReq* req = (ShootReq*)packet;
		auto player = FindPlayer(req->playerID);
		if (player) {
			auto bullet = new SCObject;
			bullet->id = _bulletIDCounter++;
			bullet->dir = Vector(0, -1);
			bullet->size = Vector(24, 8);
			bullet->pos = player->pos + Vector(28, -4);
			bullet->speed = 300;
			_bullet.push_back(bullet);

			ShootRes res;
			res.type = PacketType::SHOOT_RES;
			res.bulletID = bullet->id;
			res.pos = bullet->pos;
			SendToAll((char*)&res, sizeof(res));
		}
	}
		break;
	}
}

void MultiShoot::OnLeave(SOCKET sock)
{
	fprintf(stdout, "[%d] leaved! \n", (int)sock);
	for (auto it = _player.begin(); it != _player.end(); ++it) {
		if ((*it)->sock == sock)
		{
			PlayerHitRes res;
			res.playerID = (*it)->id;
			res.playerHP = 0;
			res.monsterID = -1;
			res.type = PacketType::PLAYER_HIT_RES;

			delete (*it);
			_player.erase(it);

			SendToAll((char*)&res, sizeof(PlayerHitRes));
			break;
		}
	}
}

void MultiShoot::SpawnEnemy()
{
	for (int i = 0; i < 5; ++i) {
		auto enemy = new SCObject;
		enemy->id = _enemyIDCounter++;
		enemy->dir = Vector(0, 1);
		enemy->hp = 5;
		enemy->size = Vector(43, 51);
		enemy->pos = Vector(120.f * i + 60.f - 21.5f, 0);
		enemy->speed = 200;
		_enemy.push_back(enemy);

		MonsterSpawnRes res;
		res.monsterID = enemy->id;
		res.pos = enemy->pos;
		res.hp = enemy->hp;
		res.type = PacketType::MONSTER_SPAWN_RES;
		SendToAll((char*)&res, sizeof(res));
	}
}

void MultiShoot::UpdatePosition(float dt)
{
	for (auto it = _player.begin(); it != _player.end();++it) {
		(*it)->pos = (*it)->pos + (*it)->dir * (*it)->speed * dt;
	}

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

bool MultiShoot::PlayerEnemyCollision(SCObject& enemy)
{
	Rect er(enemy.pos, enemy.size);
	for (auto it = _player.begin(); it != _player.end();) {
		Rect br((*it)->pos, (*it)->size);
		if (Rect::IsOverlapped(er, br)) {
			PlayerHitRes res;
			res.playerID = (*it)->id;
			res.playerHP = --(*it)->hp;
			res.monsterID = enemy.id;
			res.type = PacketType::PLAYER_HIT_RES;

			SendToAll((char*)&res, sizeof(PlayerHitRes));

			auto player = *it;
			if (player->hp <= 0) {
				GameEndRes gres;
				gres.playerID = player->id;
				gres.bestscore = _score;
				gres.score = _score - player->score;
				gres.type = PacketType::GAME_END_RES;
				send_data(player->sock, (char*)&gres, sizeof(GameEndRes));

				_player.erase(it);
				delete player;
			}

			return true;
		}
		else
			++it;
	}
	return false;
}

bool MultiShoot::BulletEnemyCollision(SCObject& enemy)
{
	Rect er(enemy.pos, enemy.size);
	for (auto it = _bullet.begin(); it != _bullet.end();) {
		Rect br((*it)->pos, (*it)->size);
		if (Rect::IsOverlapped(er, br)) {
			auto res = new MonsterHitRes;
			res->bulletID = (*it)->id;
			res->monsterHP = --enemy.hp;
			res->monsterID = enemy.id;
			res->type = PacketType::MONSTER_HIT_RES;
			
			SendToAll((char*)res, sizeof(MonsterHitRes));

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

SCObject* MultiShoot::FindPlayer(uint32_t id)
{
	for (auto p : _player)
		if (p->id == id)
			return p;
	return nullptr;
}

void MultiShoot::SendToAll(char* data, int size)
{
	for (auto p : _player) {
		this->send_data(p->sock, data, size);
	}
}

void MultiShoot::OnUpdate(float dt) {
	if (_enemySponCounter >= _enemySponTime) {
		this->SpawnEnemy();
		_enemySponCounter = 0;
	}
	this->UpdatePosition(dt);

	_enemySponCounter += dt;
}

void MultiShoot::FirstData(SOCKET sock) {
	for (auto p : _player) {
		PlayerSpawnRes res;
		res.hp = p->hp;
		res.playerID = p->id;
		res.pos = p->pos;
		res.dir = p->dir;
		res.type = PacketType::PLAYER_SPAWN_RES;
		send_data(sock, (char*)&res, sizeof(res));
	}
	for (auto e : _enemy) {
		MonsterSpawnRes res;
		res.hp = e->hp;
		res.monsterID = e->id;
		res.pos = e->pos;
		res.type = PacketType::MONSTER_SPAWN_RES;
		send_data(sock, (char*)&res, sizeof(res));
	}
	for (auto b : _bullet) {
		ShootRes res;
		res.bulletID = b->id;
		res.pos = b->pos;
		res.type = PacketType::SHOOT_RES;
		send_data(sock, (char*)&res, sizeof(res));
	}
}

const std::string currentDateTime() {
	time_t     now = time(0);
	tm			tstruct;
	char       buf[80];
	localtime_s(&tstruct, &now);
	strftime(buf, sizeof(buf), "%Y/%m/%d %X", &tstruct);

	return buf;
}

