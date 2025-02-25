#include "Bullet.hpp"
#include <stdio.h>
using namespace Tvdr;

Bullet::Bullet(Uint32 id, Vector startPos) : GameObject("1.bmp"){
	SetScale(24.f, 8.f);
	SetColor(255, 255, 0);
	
	_moveSpeed = 300;
	Init(id, startPos);
}

Bullet::~Bullet(){

}

void Bullet::Init(Uint32 id, Tvdr::Vector startPos){
	this->id = id;
	_dir = Vector(0, -1);
	SetPosition(startPos);
}

void Bullet::Update(){
	auto pos = GetPosition();
	SetPosition(pos + _dir * _moveSpeed * GameManager::GetDeltaTime());
	if (!GetRect().Overlapped(Graphics::GetScreenRect()))
		SetActive(false);
}
