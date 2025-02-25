#include "Enemy.hpp"

using namespace Tvdr;

Enemy::Enemy(Uint32 id, Vector pos, Uint32 hp)
	: GameObject("enemy.bmp"){
	SetScale(0.2f, -0.2f);
	_hpBar = new GameObject("1.bmp");
	_hpBar->SetColor(255,0,0);
	_hpBar->SetLocalPosition(0, -11);
	AddChild(_hpBar);

	_moveSpeed = 200.f;
	Init(id, pos, hp);
}

Enemy::~Enemy(){

}

void Enemy::Init(Uint32 id, Vector pos, Uint32 hp){
	this->id = id;
	SetPosition(pos);
	_hp = hp;
	UpdateHPBar();
}

bool Enemy::SetHP(Uint32 hp){
	_hp = hp;
	UpdateHPBar();
	if (_hp <= 0)
		return true;
	return false;
}

void Enemy::Update(){
	auto pos = GetPosition();
	pos.y += _moveSpeed * GameManager::GetDeltaTime();
	SetPosition(pos);

	if (pos.y > Graphics::GetScreenHeight())
		SetActive(false);
}

void Enemy::UpdateHPBar(){
	auto size = GetPrintSize();
	_hpBar->SetScale(size.x * (float)_hp / 5, 10);
}
