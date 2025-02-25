#pragma once

#include "Object.hpp"
#include <string>
#include "SDL.h"
#include "Vector.hpp"
#include "Rect.hpp"

namespace Tvdr{
	class GameObject : public Object {
		friend class Text;
	private:
		SDL_Surface* _surface;
		SDL_Texture* _texture;
		Vector _textureSize;
		Vector _anchor;
		SDL_Color _color;

		virtual void Render();
	public:
		Vector const& GetTextureSize() { return _textureSize; }
		Vector GetPrintSize() { return _textureSize * _scale.Abs(); }
		void SetColor(Uint8 r, Uint8 g, Uint8 b);
		Vector GetAnchor() { return _anchor; }
		void SetAnchor(Vector const &anchor) { _anchor = anchor; }
		Rect GetRect(){ return Rect(GetPosition(), GetPrintSize());}

		GameObject(std::string imagePath);
		virtual ~GameObject();

	protected:
		GameObject();
	};
}