#include "Player.hpp"
#include "Bullet.hpp"

using namespace Tvdr;

Player::Player(Uint32 did, Vector pos, Uint32 hp): GameObject("player.bmp") {
	SetScale(0.2f, 0.2f);
	
	auto size = GetPrintSize();
	_moveSpeed = 150.f;
	_hp = hp;
	
	_dir = Vector(0, 0);
	id = did;

	_hpBar = new GameObject("1.bmp");
	_hpBar->SetColor(255, 0, 0);
	_hpBar->SetLocalPosition(0, size.y + 10);
	AddChild(_hpBar);

	SetPosition(pos);
	
	UpdateHPBar();
}

Player::~Player(){

}

void Player::SetHP(Uint32 hp){
	_hp = hp;
	UpdateHPBar();
}


void Player::Update(){
	auto pos = GetPosition();


	SetPosition(pos + _dir * _moveSpeed * GameManager::GetDeltaTime());
}

void Player::UpdateHPBar() {
	auto size = GetPrintSize();
	_hpBar->SetScale(size.x * (float)_hp / 5, 10);
}