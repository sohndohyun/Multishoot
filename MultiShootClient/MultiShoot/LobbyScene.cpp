#include "LobbyScene.hpp"
#include "HelloWorld.hpp"
#include "tinyxml2.h"

using namespace Tvdr;
using namespace std;

LobbyScene::LobbyScene(){
	Start();
}


void LobbyScene::Start(){

	_mode = GameMode::SINGLE;
	auto background = new GameObject("background.bmp");
	background->SetScale(1.5f, 1.5f);
	AddChild(background);

	auto begintext = new Text("Plaguard-ZVnjx.ttf", 15);
	begintext->SetText("click SPACE key to start");
	begintext->SetAnchor(Vector(0.5f, 0.5f));
	begintext->SetLocalPosition(Graphics::GetScreenWidth() / 2, 700);
	AddChild(begintext);

	_single = new Text("Plaguard-ZVnjx.ttf", 30);
	_single->SetText("Single Mode");
	_single->SetAnchor(Vector(0.5f, 0.5f));
	_single->SetLocalPosition(Graphics::GetScreenWidth() / 2, 500);
	AddChild(_single);

	_multi = new Text("Plaguard-ZVnjx.ttf", 30);
	_multi->SetText("Multi Mode");
	_multi->SetAnchor(Vector(0.5f, 0.5f));
	_multi->SetLocalPosition(Graphics::GetScreenWidth() / 2, 550);
	AddChild(_multi);

	_whatSelected = new GameObject("1.bmp");
	_whatSelected->SetScale(15, 15);
	_whatSelected->SetAnchor(Vector(1, 0.5f));
	_whatSelected->SetLocalPosition(Graphics::GetScreenWidth() / 2 - _single->GetPrintSize().x / 2 - 10, 500);
	AddChild(_whatSelected);

	auto bestScoreText = new Text("Plaguard-ZVnjx.ttf", 40);
	bestScoreText->SetText("BEST " + to_string(GetBestScore()));
	bestScoreText->SetLocalPosition(20, 20);
	AddChild(bestScoreText);

	int lastScore = PlayerPref::GetInt("score");
	auto lastScoreText = new Text("Plaguard-ZVnjx.ttf", 30);
	lastScoreText->SetText("LAST " + to_string(lastScore));
	lastScoreText->SetLocalPosition(20, 70);
	AddChild(lastScoreText);
}

void LobbyScene::Update(){
	if (Input::KeyDown(SDL_SCANCODE_SPACE))
		GameManager::ChangeScene(new HelloWorld(_mode));
	if (Input::KeyDown(SDL_SCANCODE_DOWN) || Input::KeyDown(SDL_SCANCODE_UP))
		ChangeMode();
}

int LobbyScene::GetBestScore(){
	int score = PlayerPref::GetInt("score");
	int id = PlayerPref::GetInt("id");
	int bscore = PlayerPref::GetInt("bestscore");

	PlayerPref::SetInt("id", id);

	if (bscore < score) {
		PlayerPref::SetInt("bestscore", score);
		bscore = score;
	}

	PlayerPref::Save();
	return bscore;
}

void LobbyScene::ChangeMode() {
	_mode = _mode == GameMode::SINGLE ? GameMode::MULTI : GameMode::SINGLE;
	_whatSelected->SetLocalPosition(_whatSelected->GetLocalPosition().x, 
		_mode == GameMode::SINGLE ? 500 : 550);
}