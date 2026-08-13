#include "LobbyScene.hpp"

#include "HelloWorld.hpp"
#include "tinyxml2.h"

using std::to_string;
using tvdr::game_manager;
using tvdr::game_object;
using tvdr::graphics;
using tvdr::input;
using tvdr::player_pref;
using tvdr::text;
using tvdr::vector2;

lobby_scene::lobby_scene() {
    start();
}

void lobby_scene::start() {

    mode_ = game_mode::single;
    auto background = new game_object("background.bmp");
    background->set_scale(1.5f, 1.5f);
    add_child(background);

    auto begin_text = new text("Plaguard-ZVnjx.ttf", 15);
    begin_text->set_text("click SPACE key to start");
    begin_text->set_anchor(vector2(0.5f, 0.5f));
    begin_text->set_local_position(graphics::screen_width() / 2, 700);
    add_child(begin_text);

    single_ = new text("Plaguard-ZVnjx.ttf", 30);
    single_->set_text("Single Mode");
    single_->set_anchor(vector2(0.5f, 0.5f));
    single_->set_local_position(graphics::screen_width() / 2, 500);
    add_child(single_);

    multi_ = new text("Plaguard-ZVnjx.ttf", 30);
    multi_->set_text("Multi Mode");
    multi_->set_anchor(vector2(0.5f, 0.5f));
    multi_->set_local_position(graphics::screen_width() / 2, 550);
    add_child(multi_);

    selected_text_ = new game_object("1.bmp");
    selected_text_->set_scale(15, 15);
    selected_text_->set_anchor(vector2(1, 0.5f));
    selected_text_->set_local_position(
        graphics::screen_width() / 2 - single_->print_size().x / 2 - 10, 500);
    add_child(selected_text_);

    auto best_score_text = new text("Plaguard-ZVnjx.ttf", 40);
    best_score_text->set_text("BEST " + to_string(best_score()));
    best_score_text->set_local_position(20, 20);
    add_child(best_score_text);

    int last_score = player_pref::get_int("score");
    auto last_score_text = new text("Plaguard-ZVnjx.ttf", 30);
    last_score_text->set_text("LAST " + to_string(last_score));
    last_score_text->set_local_position(20, 70);
    add_child(last_score_text);
}

void lobby_scene::update() {
    if (input::is_key_down(SDL_SCANCODE_SPACE))
        game_manager::change_scene(new hello_world(mode_));
    if (input::is_key_down(SDL_SCANCODE_DOWN) || input::is_key_down(SDL_SCANCODE_UP))
        change_mode();
}

int lobby_scene::best_score() {
    int score = player_pref::get_int("score");
    int id = player_pref::get_int("id");
    int best_score_value = player_pref::get_int("best_score");

    player_pref::set_int("id", id);

    if (best_score_value < score) {
        player_pref::set_int("best_score", score);
        best_score_value = score;
    }

    player_pref::save();
    return best_score_value;
}

void lobby_scene::change_mode() {
    mode_ = mode_ == game_mode::single ? game_mode::multi : game_mode::single;
    selected_text_->set_local_position(selected_text_->local_position().x,
                                       mode_ == game_mode::single ? 500 : 550);
}
