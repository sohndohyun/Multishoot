#pragma once

#include <list>
#include <vector>
#include <string>
#include "Vector.hpp"

namespace Tvdr{

	class Object{
		friend class Scene;
		friend class GameManager;
	private:
		std::list<Object*> _children;
		Object* _parent;
		bool _needRelease;
		bool _isActive;
	protected:
		Vector _position;
		Vector _scale;
		float _rotate;

		bool AddChild(Object* obj);
		
		virtual void Update();
		virtual void Render();
	public:
		void SetParent(Object* parent);
		Object const *GetParent();

		void SetActive(bool active);
		bool GetActive();

		void Release();

		Vector GetPosition();
		Vector GetLocalPosition(){return _position;};
		void SetLocalPosition(float x, float y) {_position.x = x; _position.y = y;}
		void SetLocalPosition(Vector vec) {_position = vec;}
		void SetPosition(float x, float y);
		void SetPosition(Vector vec);

		void SetScale(float x, float y) { _scale.x = x;_scale.y = y;}
		void SetScale(Vector vec){_scale = vec;}
		Vector const &GetScale() {return _scale;}

		Object();
		virtual ~Object();
	};
}