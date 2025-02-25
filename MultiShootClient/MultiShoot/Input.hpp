#pragma once

#include "SDL.h"
#include <iostream>

namespace Tvdr{

	class Input {
	private:
		static Input *_instance;
		bool _nowState[SDL_NUM_SCANCODES];
		bool _lastState[SDL_NUM_SCANCODES];

	public :
		static Input *Instance();
		static void Release();

		static bool GetKey(SDL_Scancode keyCode);
		static bool KeyDown(SDL_Scancode keyCode);
		static bool KeyUP(SDL_Scancode keyCode);

		void UpdateKeyState();
	private :
		static void GetKeyState(SDL_Scancode scanCode, bool &nowState, bool &lastState);
		Input();
		~Input();
	};
}
