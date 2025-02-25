#include "GameObject.hpp"
#include "Graphics.hpp"
#include <iostream>
#include <string>
namespace Tvdr{
	using namespace std;

	GameObject::GameObject(string imagePath): _anchor(0, 0) {
		_scale = Vector(1, 1);

		_surface = SDL_LoadBMP((string("resource/") + imagePath).c_str());
		_texture = Graphics::GetTextureFromSurface(_surface);
		int w, h;
		SDL_QueryTexture(_texture, nullptr, nullptr, &w, &h);
		_textureSize = Vector((float)w, (float)h);
		SetColor(255, 255, 255);
	}

	GameObject::GameObject(): _anchor(0,0) {
		_scale = Vector(1, 1);
		_surface = nullptr;
		_texture = nullptr;
		_textureSize = Vector(0, 0);
		SetColor(255, 255, 255);
	}
	
	GameObject::~GameObject(){
		if (_texture)
			SDL_DestroyTexture(_texture);
		if (_surface)
			SDL_FreeSurface(_surface);
	}

	void GameObject::Render(){
		if (_texture == nullptr)
			return;

		SDL_Rect rect;
		Vector pos = GetPosition();

		rect.w = (int)(_textureSize.x * _scale.x);
		rect.h = (int)(_textureSize.y * _scale.y);

		rect.x = (int)pos.x - (rect.w * _anchor.x);
		rect.y = (int)pos.y - (rect.h * _anchor.y);
		

		Graphics::RenderTexture(_texture, &rect, _rotate);
	}

	void GameObject::SetColor(Uint8 r, Uint8 g, Uint8 b){
		_color = { r,g,b,SDL_ALPHA_OPAQUE };
		SDL_SetTextureColorMod(_texture, r, g, b);
	}
}