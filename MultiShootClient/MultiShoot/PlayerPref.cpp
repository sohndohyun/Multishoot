#include "PlayerPref.hpp"
#include "tinyxml2.h"

namespace Tvdr{
	using namespace std;
	using namespace tinyxml2;

	PlayerPref *PlayerPref::_instance = nullptr;

	PlayerPref::PlayerPref(){
		LoadData();
	}
	PlayerPref::~PlayerPref(){
		SaveData();
	}

	void PlayerPref::SaveData(){
		XMLDocument xmlDoc;

		XMLNode* root = xmlDoc.NewElement("Pref");
		xmlDoc.InsertFirstChild(root);

		for (auto it = _intDic.begin(); it != _intDic.end();++it){
			auto key = xmlDoc.NewElement("key");
			key->SetText(it->first.c_str());
			root->InsertEndChild(key);
			
			auto value = xmlDoc.NewElement("int");
			value->SetText(it->second);
			root->InsertEndChild(value);
		}

		for (auto it = _floatDic.begin(); it != _floatDic.end();++it){
			auto key = xmlDoc.NewElement("key");
			key->SetText(it->first.c_str());
			root->InsertEndChild(key);
			
			auto value = xmlDoc.NewElement("float");
			value->SetText(it->second);
			root->InsertEndChild(value);
		}

		for (auto it = _strDic.begin(); it != _strDic.end();++it){
			auto key = xmlDoc.NewElement("key");
			key->SetText(it->first.c_str());
			root->InsertEndChild(key);
			
			auto value = xmlDoc.NewElement("string");
			value->SetText(it->second.c_str());
			root->InsertEndChild(value);
		}
		xmlDoc.SaveFile("PlayerPref.xml");
	}

	void PlayerPref::LoadData(){
		XMLDocument xmlDoc;
		auto error = xmlDoc.LoadFile("PlayerPref.xml");
		if (xmlDoc.LoadFile("PlayerPref.xml") != XMLError::XML_SUCCESS)
			return;
		auto root = xmlDoc.RootElement();
		for (auto ele = root->FirstChildElement();ele != nullptr;ele = ele->NextSiblingElement()){
			auto key = ele->GetText();
			ele = ele->NextSiblingElement();
			if (string(ele->Value()) == "int"){
				int value;
				ele->QueryIntText(&value);
				_intDic[key] = value;
			}
			else if (string(ele->Value()) == "float"){
				float value;
				ele->QueryFloatText(&value);
				_floatDic[key] = value;
			}
			else{
				_strDic[key] = ele->GetText();
			}
		}
	}

	PlayerPref *PlayerPref::Instance(){
		if (!_instance)
			_instance = new PlayerPref;
		return _instance;
	}

	void PlayerPref::Release(){
		delete _instance;
	}

	void PlayerPref::DeleteKey(string const &key){
		auto pref = Instance();
		if (pref->_strDic.find(key) != pref->_strDic.end())
			pref->_strDic.erase(key);
		else if (pref->_floatDic.find(key) != pref->_floatDic.end())
			pref->_floatDic.erase(key);
		else if (pref->_intDic.find(key) != pref->_intDic.end())
			pref->_intDic.erase(key);
	}

	void PlayerPref::DeleteAll(){
		auto pref = Instance();
		pref->_strDic.clear();
		pref->_floatDic.clear();
		pref->_intDic.clear();
	}

	float PlayerPref::GetFloat(string const &key, float def){
		auto pref = Instance();
		if (pref->_floatDic.find(key) != pref->_floatDic.end())
			return pref->_floatDic[key];
		return def;
	}

	int PlayerPref::GetInt(string const &key, int def){
		auto pref = Instance();
		if (pref->_intDic.find(key) != pref->_intDic.end())
			return pref->_intDic[key];
		return def;
	}

	string const &PlayerPref::GetString(string const &key, string const &def){
		auto pref = Instance();
		if (pref->_strDic.find(key) != pref->_strDic.end())
			return pref->_strDic[key];
		return def;
	}

	bool PlayerPref::HasKey(string const &key){
		auto pref = Instance();
		if (pref->_strDic.find(key) != pref->_strDic.end())
			return true;
		else if (pref->_floatDic.find(key) != pref->_floatDic.end())
			return true;
		else if (pref->_intDic.find(key) != pref->_intDic.end())
			return true;
		else
			return false;
	}

	void PlayerPref::Save(){
		Instance()->SaveData();
	}

	void PlayerPref::SetFloat(string const &key, float value){
		Instance()->DeleteKey(key);
		Instance()->_floatDic[key] = value;
	}

	void PlayerPref::SetInt(string const &key, int value){
		Instance()->DeleteKey(key);
		Instance()->_intDic[key] = value;
	}

	void PlayerPref::SetString(string const &key, string const &value){
		Instance()->DeleteKey(key);
		Instance()->_strDic[key] = value;
	}
}