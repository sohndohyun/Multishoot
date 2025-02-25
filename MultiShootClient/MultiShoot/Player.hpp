#pragma once

#include "TVDR.hpp"

class Player : public Tvdr::GameObject{
public:
	Player(Uint32 id, Tvdr::Vector pos, Uint32 hp);
	virtual ~Player();

	void SetHP(Uint32 hp);
	int GetHP(){return _hp;}
	void SetDir(Tvdr::Vector const& dir) { _dir = dir; }
	Tvdr::Vector const& GetDir() { return _dir; }

protected:
	virtual void Update();
	void UpdateHPBar();
private:
	float _moveSpeed;
	Tvdr::Vector _dir;
	Uint32 _hp;
	Tvdr::GameObject* _hpBar;
public:
	Uint32 id;
};