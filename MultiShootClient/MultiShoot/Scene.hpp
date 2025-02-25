#pragma once

#include "Object.hpp"

namespace Tvdr{
	class Scene : public Object{
		friend class GameManager;
	private:
		void UpdateAll(Object* obj = nullptr);
		void ReleaseAll(Object* obj = nullptr);
	public:
		Scene();
		virtual ~Scene();
	};
}