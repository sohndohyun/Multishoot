#pragma once

#include <map>
#include <string>

class player_pref {
  private:
    static player_pref* instance_;

    player_pref();
    ~player_pref();

    std::map<std::string, std::string> string_values_;
    std::map<std::string, float> float_values_;
    std::map<std::string, int> int_values_;

    void save_data();
    void load_data();

  public:
    static player_pref* instance();
    static void release();

    static void delete_key(std::string const& key);
    static void delete_all();

    static float get_float(std::string const& key, float def = 0);
    static int get_int(std::string const& key, int def = 0);
    static std::string const& get_string(std::string const& key, std::string const& def = " ");

    static bool has_key(std::string const& key);
    static void save();

    static void set_float(std::string const& key, float value);
    static void set_int(std::string const& key, int value);
    static void set_string(std::string const& key, std::string const& value);
};
