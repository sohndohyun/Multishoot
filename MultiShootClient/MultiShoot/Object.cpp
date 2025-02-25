#include "Object.hpp"
#include <algorithm>
#include <iostream>
#include "GameManager.hpp"

using namespace std;

namespace Tvdr{

	Object::Object() : _parent(nullptr), _needRelease(false), _isActive(true), _rotate(0){}
	Object::~Object(){
		if (_parent)
			_parent->_children.remove(this);
		while (!_children.empty())
			delete _children.front();
	}

	bool Object::AddChild(Object *obj){
		if (find(_children.begin(), _children.end(), obj) == _children.end()){
			_children.push_back(obj);
			obj->_parent = this;
			obj->SetPosition(obj->GetLocalPosition());
			return true;
		}
		return false;
	}

	void Object::Update(){}
	void Object::Render(){}

	void Object::SetParent(Object* parent){
		if (_parent)
			_parent->_children.remove(this);
		_parent = parent;
		parent->AddChild(this);
	}

	Object const *Object::GetParent(){
		return _parent;
	}

	void Object::SetActive(bool active){
		_isActive = active;
	}

	bool Object::GetActive(){
		return _isActive;
	}

	void Object::Release(){
		_needRelease = true;
	}

	Vector Object::GetPosition(){
		if (!_parent)
			return _position;
		return _parent->GetPosition() + _position;
	}

	void Object::SetPosition(float x, float y){
		if (!_parent)
			SetLocalPosition(x, y);
		else
			SetLocalPosition(Vector(x, y) - _parent->GetPosition());
	}

	void Object::SetPosition(Vector vec){
		if (!_parent)
			SetLocalPosition(vec);
		else
			SetLocalPosition(vec - _parent->GetPosition());
	}
}