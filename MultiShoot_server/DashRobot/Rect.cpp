#include "Rect.hpp"
#include <algorithm>

namespace Tvdr{
	using namespace std;

	Rect::Rect(float x, float y, float w, float h) : pos(x, y), size(w, h){
		SetSize(size);
	}
	Rect::Rect(Vector pos, Vector size) : pos(pos){
		SetSize(size);
	}

	void Rect::SetSize(Vector size){
		this->size.x = max(size.x, 0.f);
		this->size.y = max(size.y, 0.f);
	}

	bool Rect::IsOverlapped(Rect const &rect1, Rect const &rect2) {
		if (rect1.pos.x > rect2.pos.x + rect2.size.x ||
			rect2.pos.x > rect1.pos.x + rect1.size.x ||
			rect1.pos.y > rect2.pos.y + rect2.size.y ||
			rect2.pos.y > rect1.pos.y + rect1.size.y)
			return false;
		return true;
	}
	bool Rect::Overlapped(Rect const &r){
		return IsOverlapped(*this, r);
	}
}