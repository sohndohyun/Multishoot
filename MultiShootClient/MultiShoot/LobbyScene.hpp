#pragma once

#include "TVDR.hpp"

enum class GameMode {
	SINGLE, MULTI
};

class LobbyScene : public Tvdr::Scene{
protected:
	virtual void Update();
	void Start();

	int GetBestScore();

	Tvdr::Text* _single;
	Tvdr::Text* _multi;
	Tvdr::GameObject* _whatSelected;
	GameMode _mode;
	void ChangeMode();
public:
	LobbyScene();
	virtual ~LobbyScene(){}

};