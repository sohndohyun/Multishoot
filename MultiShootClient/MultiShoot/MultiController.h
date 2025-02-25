#pragma once
#include "GameController.h"
#include "MultiShootClient.h"

class MultiController : public GameController
{
protected:
	virtual void Update();
public:
	MultiController();
	virtual ~MultiController();

	virtual void ChangeDir(Tvdr::Vector dir, Uint32 id);
	virtual void Shoot(Uint32 id);
	virtual bool Work() { return _work; }

private:
	MultiShootClient _tool;
	bool _work;
};
