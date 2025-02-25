#include "Input.hpp"
#include <Windows.h>

namespace Tvdr {
	Input *Input::_instance = nullptr;
	Input *Input::Instance() {
		if (_instance == nullptr)
			_instance = new Input();
		return _instance;
	}

	void Input::Release() {
		delete _instance;
		_instance = nullptr;
	}

	Input::Input() {
		for (int i = 0; i < SDL_NUM_SCANCODES; ++i) {
			_lastState[i] = false;
			_nowState[i] = false;
		}
	}

	Input::~Input() {
	}

	bool Input::KeyDown(SDL_Scancode keyCode) {
		bool lastState;
		bool nowState;
		GetKeyState(keyCode, nowState, lastState);
		return (!lastState && nowState) ? true : false;
	}

	bool Input::GetKey(SDL_Scancode keyCode) {
		bool lastState;
		bool nowState;
		GetKeyState(keyCode, nowState, lastState);
		return nowState ? true : false;
	}

	bool Input::KeyUP(SDL_Scancode keyCode) {
		bool lastState;
		bool nowState;
		GetKeyState(keyCode, nowState, lastState);
		return (lastState && !nowState) ? true : false;
	}

	void Input::UpdateKeyState() {
		auto keyStates = SDL_GetKeyboardState(nullptr);
		for (int i = 0; i < SDL_NUM_SCANCODES; ++i) {
			_lastState[i] = _nowState[i];
			_nowState[i] = keyStates[i] ? true : false;
		}
	}

	void Input::GetKeyState(SDL_Scancode scanCode, bool& nowState, bool& lastState) {
		nowState = _instance->_nowState[scanCode];
		lastState = _instance->_lastState[scanCode];
	}
}