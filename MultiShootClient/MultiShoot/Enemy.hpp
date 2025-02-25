#pragma once

#include "TVDR.hpp"

class Enemy : public Tvdr::GameObject{
public:
	Enemy(Uint32 id, Tvdr::Vector pos, Uint32 hp);
	virtual ~Enemy();
	void Init(Uint32 id, Tvdr::Vector pos, Uint32 hp);

	bool SetHP(Uint32 hp);

	Uint32 id;
protected:
	virtual void Update();
	void UpdateHPBar();
private:
	float _moveSpeed;

	Uint32 _hp;
	Tvdr::GameObject* _hpBar;
};