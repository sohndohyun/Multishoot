#include "Text.hpp"
#include "Graphics.hpp"
#include <iostream>

namespace Tvdr{
	using namespace std;

	Text::Text(string ttfPath, int size): GameObject() {
		_font = TTF_OpenFont((string("resource/") + ttfPath).c_str(), size);
		_size = size;
		_text = " ";
	
		InitTexture();
	}

	Text::~Text(){
		if (_font)
			TTF_CloseFont(_font);
	}

	void Text::InitTexture(){
		if (_surface)
			SDL_FreeSurface(_surface);
		if (_texture)
			SDL_DestroyTexture(_texture);

		_surface = TTF_RenderText_Blended(_font, _text.c_str(), _color);
		if (_surface == nullptr)
			return;

		_texture = Graphics::GetTextureFromSurface(_surface);
		int w, h;
		SDL_QueryTexture(_texture, nullptr, nullptr, &w, &h);
		_textureSize.x = (float)w;
		_textureSize.y = (float)h;
	}

	void Text::SetText(string text){
		_text = text;
		InitTexture();
	}
}