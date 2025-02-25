#include "Scene.hpp"

namespace Tvdr{

	Scene::Scene(){

	}

	Scene::~Scene(){
		
	}

	void Scene::UpdateAll(Object* obj){
		if (obj == nullptr)
			obj = this;
		
		if (obj->GetActive() == false)
			return;
			
		obj->Update();
		if (obj->GetActive() == false)
			return;
		obj->Render();

		for (auto child : obj->_children){
			UpdateAll(child);
		}
	}

	void Scene::ReleaseAll(Object* obj){
		if (obj == nullptr)
			obj = this;

		for (auto it = obj->_children.begin(); it != obj->_children.end();){
			auto child = *it;
			if (child->_needRelease)
			{
				it = obj->_children.erase(it);
				delete child;
			}
			else{
				ReleaseAll(child);
				++it;
			}
		}
	}
}