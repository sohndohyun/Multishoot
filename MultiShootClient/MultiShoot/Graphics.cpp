#include "Graphics.hpp"
#include "SDL_ttf.h"
#include <iostream>

namespace Tvdr{

	Graphics *Graphics::sInstance = nullptr;
	bool Graphics::sInitialized = false;

	Graphics* Graphics::Instance() {
		if (sInstance == nullptr)
			sInstance = new Graphics();
		return sInstance;
	}

	Graphics* Graphics::Instance(Vector screenSize, std::string const& windowName) {
		if (sInstance == nullptr)
			sInstance = new Graphics(screenSize, windowName);
		return sInstance;
	}

	void Graphics::Realase() {
		delete sInstance;
		sInstance = nullptr;

		sInitialized = false;
	}

	bool Graphics::Initialized() {
		return sInitialized;
	}

	Graphics::Graphics() {
		_screenSize = Vector(800, 600);
		_windowName = "Tvdr Project";
		_renderer = nullptr;
		sInitialized = Init();
	}

	Graphics::Graphics(Vector screenSize, std::string const& windowName) {
		_screenSize = screenSize;
		_windowName = windowName;
		_renderer = nullptr;
		sInitialized = Init();
	}

	Graphics::~Graphics() {
		SDL_DestroyWindow(_window);
		_window = nullptr;
		SDL_DestroyRenderer(_renderer);
		TTF_Quit();
		SDL_Quit();
	}

	bool Graphics::Init() {
		if(SDL_Init(SDL_INIT_VIDEO) < 0) {
			printf("SDL InitialIzetion Error:%s\n", SDL_GetError());
			return false;
		}
		
		if(TTF_Init() < 0) {
			printf("SDL_TTF InitialIzetion Error:%s\n", TTF_GetError());
			return false;
		}

		_window = SDL_CreateWindow(_windowName.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (int)_screenSize.x, (int)_screenSize.y, SDL_WINDOW_SHOWN);
	
		if (_window == nullptr) {
			printf("window creation ERROR : %s\n", SDL_GetError());
			return false;
		}

		_renderer = SDL_CreateRenderer(_window, -1, 0);
		if (_renderer == nullptr) {
			printf("renderer creation ERROR : %s\n", SDL_GetError());
			return false;
		}

		return true;
	}

	void Graphics::ClearRenderer(){
		SDL_RenderClear(_renderer);
	}

	void Graphics::Render() {
		SDL_RenderPresent(_renderer);
		SDL_UpdateWindowSurface(_window);
	}

	SDL_Texture* Graphics::GetTextureFromSurface(SDL_Surface* surface){
		return SDL_CreateTextureFromSurface(Instance()->_renderer, surface);
	}

	void Graphics::RenderTexture(SDL_Texture* texture, SDL_Rect* drect, float rotate){
		auto renderer = Instance()->_renderer;
		int flip = SDL_FLIP_NONE;
		if (drect->h < 0){
			flip |= SDL_FLIP_VERTICAL;
			drect->h = -drect->h;
		}
		if (drect->w < 0){
			flip |= SDL_FLIP_HORIZONTAL;
			drect->w = -drect->w;
		}
		SDL_RenderCopyEx(renderer, texture, nullptr, drect, rotate, nullptr, (SDL_RendererFlip)flip);
	}
};