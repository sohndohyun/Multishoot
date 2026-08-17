#include "scenes/auth_scene.hpp"

#include "engine/game_manager.hpp"
#include "engine/graphics.hpp"
#include "engine/input.hpp"
#include "scenes/hello_world.hpp"
#include "scenes/lobby_scene.hpp"

#include <algorithm>
#include <utility>

using dr::vector2;

namespace {

bool username_character(char value) {
    return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
           (value >= '0' && value <= '9') || value == '_';
}

} // namespace

auth_scene::auth_scene() {
    auto* background = new game_object("background.bmp");
    background->set_scale(1.5f, 1.5f);
    add_child(background);

    mode_text_ = make_text(26, 270);
    username_text_ = make_text(25, 370);
    password_text_ = make_text(25, 420);
    confirmation_text_ = make_text(25, 470);
    status_text_ = make_text(18, 620);
    help_text_ = make_text(15, 680);
    for (int row = 0; row <= 10; ++row)
        leaderboard_texts_.push_back(make_text(row == 0 ? 20 : 18, 320.f + row * 27.f));

    SDL_StartTextInput();
    connect();
    refresh();
}

auth_scene::~auth_scene() {
    SDL_StopTextInput();
}

void auth_scene::connect() {
    client_ = std::make_unique<multi_shoot_client>();
    if (client_->init("127.0.0.1", 3000) != 0 || !client_->start()) {
        client_.reset();
        status_ = "Connection failed. Press ENTER to retry.";
    } else {
        status_ = "Connected.";
    }
    pending_ = false;
    leaderboard_loaded_ = false;
}

void auth_scene::update() {
    if (input::is_key_down(SDL_SCANCODE_ESCAPE)) {
        game_manager::change_scene(new lobby_scene);
        return;
    }

    if (!client_) {
        if (input::is_key_down(SDL_SCANCODE_RETURN)) {
            connect();
            refresh();
        }
        return;
    }

    handle_response();
    if (!client_)
        return;
    if (!client_->is_working()) {
        client_.reset();
        status_ = "Disconnected. Press ENTER to retry.";
        refresh();
        return;
    }
    if (pending_)
        return;

    if (input::is_key_down(SDL_SCANCODE_LEFT)) {
        mode_ = mode_ == mode::login ? mode::leaderboard
                                     : mode_ == mode::signup ? mode::login : mode::signup;
    } else if (input::is_key_down(SDL_SCANCODE_RIGHT)) {
        mode_ = mode_ == mode::login ? mode::signup
                                     : mode_ == mode::signup ? mode::leaderboard : mode::login;
    }

    if (mode_ == mode::leaderboard) {
        if (input::is_key_down(SDL_SCANCODE_UP) && leaderboard_page_ > 0)
            request_leaderboard(leaderboard_page_ - 1);
        else if (input::is_key_down(SDL_SCANCODE_DOWN) && has_next_page_)
            request_leaderboard(leaderboard_page_ + 1);
        else if (!leaderboard_loaded_ || input::is_key_down(SDL_SCANCODE_RETURN))
            request_leaderboard(leaderboard_page_);
        refresh();
        return;
    }

    field_ = (std::min)(field_, mode_ == mode::login ? 1 : 2);
    const int field_count = mode_ == mode::login ? 2 : 3;
    if (input::is_key_down(SDL_SCANCODE_TAB) || input::is_key_down(SDL_SCANCODE_DOWN))
        field_ = (field_ + 1) % field_count;
    if (input::is_key_down(SDL_SCANCODE_UP))
        field_ = (field_ + field_count - 1) % field_count;

    auto& value = current_value();
    for (char character : input::text_input()) {
        const bool allowed = field_ == 0 ? username_character(character)
                                         : character >= 33 && character <= 126;
        const std::size_t limit = field_ == 0 ? 16 : 64;
        if (allowed && value.size() < limit)
            value += character;
    }
    if (input::is_key_down(SDL_SCANCODE_BACKSPACE) && !value.empty())
        value.pop_back();
    if (input::is_key_down(SDL_SCANCODE_RETURN))
        submit();

    refresh();
}

void auth_scene::submit() {
    if (username_.size() < 3 || password_.size() < 8) {
        status_ = "Username: 3-16, password: 8-64 characters.";
        return;
    }
    if (mode_ == mode::signup && password_ != confirmation_) {
        status_ = "Passwords do not match.";
        return;
    }

    multishoot::protocol::ClientPacket packet;
    if (mode_ == mode::login) {
        auto* request = packet.mutable_login_request();
        request->set_username(username_);
        request->set_password(password_);
    } else {
        auto* request = packet.mutable_signup_request();
        request->set_username(username_);
        request->set_password(password_);
    }
    client_->send(packet);
    pending_ = true;
    status_ = "Please wait...";
}

void auth_scene::request_leaderboard(std::uint32_t page) {
    multishoot::protocol::ClientPacket packet;
    packet.mutable_leaderboard_request()->set_page(page);
    client_->send(packet);
    pending_ = true;
    status_ = "Loading leaderboard...";
}

void auth_scene::handle_response() {
    multishoot::protocol::ServerPacket packet;
    if (!client_->data_channel_.try_receive(packet))
        return;
    if (packet.payload_case() == multishoot::protocol::ServerPacket::kLeaderboardResponse) {
        pending_ = false;
        leaderboard_loaded_ = true;
        const auto& response = packet.leaderboard_response();
        if (!response.success()) {
            status_ = "Could not load leaderboard. Press ENTER to retry.";
            refresh();
            return;
        }
        leaderboard_page_ = response.page();
        has_next_page_ = response.has_next_page();
        leaderboard_rows_.clear();
        for (const auto& entry : response.entries()) {
            leaderboard_rows_.push_back(std::to_string(entry.rank()) + "    " +
                                        entry.username() + "    " +
                                        std::to_string(entry.score()));
        }
        status_ = "Page " + std::to_string(leaderboard_page_ + 1);
        refresh();
        return;
    }
    if (packet.payload_case() != multishoot::protocol::ServerPacket::kAuthResponse)
        return;

    pending_ = false;
    switch (packet.auth_response().result()) {
    case multishoot::protocol::AUTH_RESULT_SUCCESS:
        game_manager::change_scene(
            new hello_world(std::move(client_), packet.auth_response().best_score()));
        return;
    case multishoot::protocol::AUTH_RESULT_INVALID_INPUT:
        status_ = "Invalid username or password format.";
        break;
    case multishoot::protocol::AUTH_RESULT_USERNAME_TAKEN:
        status_ = "Username is already taken.";
        break;
    case multishoot::protocol::AUTH_RESULT_INVALID_CREDENTIALS:
        status_ = "Invalid username or password.";
        break;
    case multishoot::protocol::AUTH_RESULT_ACCOUNT_IN_USE:
        status_ = "Account is already connected.";
        break;
    default:
        status_ = "Server error. Please try again.";
        break;
    }
    refresh();
}

void auth_scene::refresh() {
    const std::string marker[] = {"  ", "> "};
    if (mode_ == mode::login)
        mode_text_->set_text("> LOGIN    SIGN UP    LEADERBOARD");
    else if (mode_ == mode::signup)
        mode_text_->set_text("LOGIN    > SIGN UP    LEADERBOARD");
    else
        mode_text_->set_text("LOGIN    SIGN UP    > LEADERBOARD");

    const bool leaderboard = mode_ == mode::leaderboard;
    username_text_->set_active(!leaderboard);
    password_text_->set_active(!leaderboard);
    confirmation_text_->set_active(mode_ == mode::signup);
    username_text_->set_text(marker[field_ == 0] + "Username: " + username_);
    password_text_->set_text(marker[field_ == 1] + "Password: " +
                             std::string(password_.size(), '*'));
    if (mode_ == mode::signup)
        confirmation_text_->set_text(marker[field_ == 2] + "Confirm: " +
                                     std::string(confirmation_.size(), '*'));
    for (std::size_t row = 0; row < leaderboard_texts_.size(); ++row) {
        leaderboard_texts_[row]->set_active(leaderboard);
        if (row == 0)
            leaderboard_texts_[row]->set_text("RANK    ID    SCORE");
        else if (row <= leaderboard_rows_.size())
            leaderboard_texts_[row]->set_text(leaderboard_rows_[row - 1]);
        else if (row == 1 && !pending_)
            leaderboard_texts_[row]->set_text("No rankings yet.");
        else
            leaderboard_texts_[row]->set_text(" ");
    }
    status_text_->set_text(status_.empty() ? " " : status_);
    help_text_->set_text(leaderboard ? "UP DOWN page  ENTER refresh  LEFT RIGHT menu  ESC back"
                                     : "ARROWS OR TAB select  ENTER submit  ESC back");
}

std::string& auth_scene::current_value() {
    if (field_ == 0)
        return username_;
    if (field_ == 1)
        return password_;
    return confirmation_;
}

text* auth_scene::make_text(int size, float y) {
    auto* value = new text("Plaguard-ZVnjx.ttf", size);
    value->set_anchor({0.5f, 0.5f});
    value->set_local_position(graphics::screen_width() / 2, y);
    add_child(value);
    return value;
}
