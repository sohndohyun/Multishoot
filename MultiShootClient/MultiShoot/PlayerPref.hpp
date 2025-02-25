#pragma once

#include <map>
#include <string>

namespace Tvdr{
	class PlayerPref{
	private:
		static PlayerPref *_instance;

		PlayerPref();
		~PlayerPref();

		std::map<std::string, std::string> _strDic;
		std::map<std::string, float> _floatDic;
		std::map<std::string, int> _intDic;

		void SaveData();
		void LoadData();
	public:
		static PlayerPref *Instance();
		static void Release();

		static void DeleteKey(std::string const &key);
		static void DeleteAll();

		static float GetFloat(std::string const &key, float def = 0);
		static int GetInt(std::string const &key, int def = 0);
		static std::string const &GetString(std::string const &key, std::string const &def = " ");

		static bool HasKey(std::string const &key);
		static void Save();

		static void SetFloat(std::string const &key, float value);
		static void SetInt(std::string const &key, int value);
		static void SetString(std::string const &key, std::string const &value);
	};
}