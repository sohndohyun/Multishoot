#pragma once
#include "Vector.hpp"

namespace Tvdr{
	class Rect {
	private:
		Vector size;
	public:
		Vector pos;
		Rect(float x = 0, float y = 0, float w = 0, float h = 0);
		Rect(Vector pos, Vector size);

		void SetSize(Vector size);
		const Vector &GetSize(){return size;}
		static bool IsOverlapped(Rect const &rect1, Rect const &rect2);
		bool Overlapped(Rect const &r);
	};
}