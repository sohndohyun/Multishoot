#include "GameManager.hpp"
#include <iostream>
#include <Windows.h>

#define MS_PER_FRAME 10

namespace Tvdr {
	GameManager *GameManager::_instance = nullptr;
	GameManager *GameManager::Instance() {
		if (_instance == nullptr)
			_instance = new GameManager();
		return _instance;
	}

	GameManager* GameManager::Instance(Vector screenSize, std::string const& windowName) {
		if (_instance == nullptr)
			_instance = new GameManager(screenSize, windowName);
		return _instance;
	}

	void GameManager::Release() {
		delete _instance;
		_instance = nullptr;
	}

	GameManager::GameManager() {
		_quit = false;
		_graphics = Graphics::Instance();
		_saveData = PlayerPref::Instance();
		_curScene = nullptr;
		_nxtScene = nullptr;
		_Input = Input::Instance();

		curTime = 0;
		stTime = 0;
		lastTime = 0;
		if (!Graphics::Initialized())
			_quit = true;
	}

	GameManager::GameManager(Vector screenSize, std::string const& windowName) {
		_quit = false;
		_graphics = Graphics::Instance(screenSize, windowName);
		_saveData = PlayerPref::Instance();
		_curScene = nullptr;
		_nxtScene = nullptr;
		_Input = Input::Instance();

		curTime = 0;
		stTime = 0;
		lastTime = 0;
		if (!Graphics::Initialized())
			_quit = true;
	}

	GameManager::~GameManager() {
		if (_curScene)
			delete _curScene;
		if (_nxtScene)
			delete _nxtScene;
		Graphics::Realase();
		PlayerPref::Release();
		Input::Release();
		_graphics = nullptr;
		_saveData = nullptr;
		_Input = nullptr;
	}

	bool GameManager::ChangeScene(Scene* scene){
		auto gm = Instance();
		if (gm->_nxtScene)
			return false;
		gm->_nxtScene = scene;
		return true;
	}

	void GameManager::Init(Vector screenSize, std::string const& windowName) {
		Instance(screenSize, windowName);
	}

	int GameManager::Run(Scene *scene) {
		if (!scene)
			return 1;
		Instance()->stTime = SDL_GetTicks64();
		Instance()->_curScene = scene;
		Instance()->MainLoop();
		GameManager::Release();
		return 0;
	}

	void GameManager::MainLoop() {
		if (!_curScene)
			return;
	
		curTime = lastTime = stTime;
		SDL_Event ev;
		while (!_quit) {
			curTime = SDL_GetTicks64();
			_graphics->ClearRenderer();
			while(SDL_PollEvent(&ev) != 0) {
				switch (ev.type)
				{
				case SDL_QUIT:
					_quit = true;
					break;
				default:
					break;
				}
			}

			_Input->UpdateKeyState();
			_curScene->UpdateAll();
			_graphics->Render();
			_curScene->ReleaseAll();

			if (_nxtScene){
				delete _curScene;
				_curScene = _nxtScene;
				_nxtScene = nullptr;
			}
			Sleep(MS_PER_FRAME - (curTime - SDL_GetTicks64()));
			lastTime = curTime;
		}
	}

	long long GameManager::GetTime() {
		return Instance()->curTime - Instance()->stTime;
	}

	float GameManager::GetDeltaTime() {
		auto deltaTime = Instance()->curTime - Instance()->lastTime;
		return (float)deltaTime / 1000.f;
	}

	bool GameManager::AddObject(Object* obj){
		if (_instance->_nxtScene)
			return _instance->_nxtScene->AddChild(obj);
		if(_instance->_curScene)
			return _instance->_curScene->AddChild(obj);
		return false;
	}
}