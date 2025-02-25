#pragma once
#include "Graphics.hpp"
#include "Scene.hpp"
#include "Input.hpp"
#include "PlayerPref.hpp"
#include <list>

namespace Tvdr{
	class GameManager{
		friend class Object;
	private:
		static GameManager *_instance;
		Input *_Input;
		PlayerPref *_saveData;
		Graphics *_graphics;
		
		
		bool _quit;
		Scene *_curScene;
		Scene *_nxtScene;

		Uint64 stTime;
		Uint64 curTime;
		Uint64 lastTime;
	public:
		static GameManager *Instance();
		static GameManager* Instance(Vector screenSize, std::string const& windowName);
		static void Release();
		static bool ChangeScene(Scene *scene);
		static void Init(Vector screenSize, std::string const &windowName);
		static int Run(Scene *scene);
		static long long GetTime(void);
		static float GetDeltaTime(void);
		static bool AddObject(Object* obj);
		static void Quit(){Instance()->_quit = true;}
	private:
		void MainLoop();

		GameManager();
		GameManager(Vector screenSize, std::string const& windowName);
		~GameManager();
	};
}