#pragma once

#include "SDL.h"
#include "Rect.hpp"
#include <string>

namespace Tvdr{
	
	class Graphics {
	private:
		static Graphics *sInstance;
		static bool sInitialized;

		SDL_Window *_window;
		SDL_Renderer* _renderer;

	public:
		static Graphics *Instance();
		static Graphics* Instance(Vector screenSize, std::string const& windowName);
		static void Realase();
		static bool Initialized();

		void ClearRenderer();
		void Render();

		static Vector GetScreenSize() {return sInstance->_screenSize;}
		static float GetScreenWidth() {return sInstance->_screenSize.x;}
		static float GetScreenHeight() {return sInstance->_screenSize.y;}
		static Rect GetScreenRect() {return Rect(0, 0, sInstance->_screenSize.x, sInstance->_screenSize.y);}
		static SDL_Texture* GetTextureFromSurface(SDL_Surface* surface);
		static void RenderTexture(SDL_Texture* texture, SDL_Rect* drect, float rotate); 
	
	private:
		Graphics();
		Graphics(Vector screenSize, std::string const& windowName);
		~Graphics();

		bool Init();
		Vector _screenSize;
		std::string _windowName;
	};
}
