#pragma once
#include "TVDR.hpp"

class Bullet : public Tvdr::GameObject{
public:
	Bullet(Uint32 id, Tvdr::Vector startPos);
	virtual ~Bullet();

	void Init(Uint32 id, Tvdr::Vector startPos);

	Uint32 id;
protected:
	virtual void Update();
private:
	Tvdr::Vector _dir;
	float _moveSpeed;
};