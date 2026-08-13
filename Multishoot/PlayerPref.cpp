#include "PlayerPref.hpp"

#include "tinyxml2.h"

namespace tvdr {
using std::string;
using tinyxml2::XMLDocument;
using tinyxml2::XMLError;
using tinyxml2::XMLNode;

player_pref* player_pref::instance_ = nullptr;

player_pref::player_pref() {
    load_data();
}
player_pref::~player_pref() {
    save_data();
}

void player_pref::save_data() {
    XMLDocument xml_doc;

    XMLNode* root = xml_doc.NewElement("Pref");
    xml_doc.InsertFirstChild(root);

    for (auto it = int_values_.begin(); it != int_values_.end(); ++it) {
        auto key = xml_doc.NewElement("key");
        key->SetText(it->first.c_str());
        root->InsertEndChild(key);

        auto value = xml_doc.NewElement("int");
        value->SetText(it->second);
        root->InsertEndChild(value);
    }

    for (auto it = float_values_.begin(); it != float_values_.end(); ++it) {
        auto key = xml_doc.NewElement("key");
        key->SetText(it->first.c_str());
        root->InsertEndChild(key);

        auto value = xml_doc.NewElement("float");
        value->SetText(it->second);
        root->InsertEndChild(value);
    }

    for (auto it = string_values_.begin(); it != string_values_.end(); ++it) {
        auto key = xml_doc.NewElement("key");
        key->SetText(it->first.c_str());
        root->InsertEndChild(key);

        auto value = xml_doc.NewElement("string");
        value->SetText(it->second.c_str());
        root->InsertEndChild(value);
    }
    xml_doc.SaveFile("player_pref.xml");
}

void player_pref::load_data() {
    XMLDocument xml_doc;
    if (xml_doc.LoadFile("player_pref.xml") != XMLError::XML_SUCCESS)
        return;
    auto root = xml_doc.RootElement();
    for (auto ele = root->FirstChildElement(); ele != nullptr; ele = ele->NextSiblingElement()) {
        auto key = ele->GetText();
        ele = ele->NextSiblingElement();
        if (string(ele->Value()) == "int") {
            int value;
            ele->QueryIntText(&value);
            int_values_[key] = value;
        } else if (string(ele->Value()) == "float") {
            float value;
            ele->QueryFloatText(&value);
            float_values_[key] = value;
        } else {
            string_values_[key] = ele->GetText();
        }
    }
}

player_pref* player_pref::instance() {
    if (!instance_)
        instance_ = new player_pref;
    return instance_;
}

void player_pref::release() {
    delete instance_;
    instance_ = nullptr;
}

void player_pref::delete_key(string const& key) {
    auto pref = instance();
    if (pref->string_values_.find(key) != pref->string_values_.end())
        pref->string_values_.erase(key);
    else if (pref->float_values_.find(key) != pref->float_values_.end())
        pref->float_values_.erase(key);
    else if (pref->int_values_.find(key) != pref->int_values_.end())
        pref->int_values_.erase(key);
}

void player_pref::delete_all() {
    auto pref = instance();
    pref->string_values_.clear();
    pref->float_values_.clear();
    pref->int_values_.clear();
}

float player_pref::get_float(string const& key, float def) {
    auto pref = instance();
    if (pref->float_values_.find(key) != pref->float_values_.end())
        return pref->float_values_[key];
    return def;
}

int player_pref::get_int(string const& key, int def) {
    auto pref = instance();
    if (pref->int_values_.find(key) != pref->int_values_.end())
        return pref->int_values_[key];
    return def;
}

string const& player_pref::get_string(string const& key, string const& def) {
    auto pref = instance();
    if (pref->string_values_.find(key) != pref->string_values_.end())
        return pref->string_values_[key];
    return def;
}

bool player_pref::has_key(string const& key) {
    auto pref = instance();
    if (pref->string_values_.find(key) != pref->string_values_.end())
        return true;
    else if (pref->float_values_.find(key) != pref->float_values_.end())
        return true;
    else if (pref->int_values_.find(key) != pref->int_values_.end())
        return true;
    else
        return false;
}

void player_pref::save() {
    instance()->save_data();
}

void player_pref::set_float(string const& key, float value) {
    instance()->delete_key(key);
    instance()->float_values_[key] = value;
}

void player_pref::set_int(string const& key, int value) {
    instance()->delete_key(key);
    instance()->int_values_[key] = value;
}

void player_pref::set_string(string const& key, string const& value) {
    instance()->delete_key(key);
    instance()->string_values_[key] = value;
}
} // namespace tvdr
