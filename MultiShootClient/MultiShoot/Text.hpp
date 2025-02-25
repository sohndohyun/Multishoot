#pragma once

#include "GameObject.hpp"
#include <string>
#include "SDL_ttf.h"

namespace Tvdr{
	class Text : public GameObject{
	private:
		std::string _text;

		unsigned int _size;
		TTF_Font* _font;

		void InitTexture();

	public:
		Text(std::string ttfPath, int size = 10);

		void SetText(std::string text);

		virtual ~Text();
	};
}