#pragma once

#include "controllers/multi_shoot_client.hpp"
#include "engine/scene.hpp"
#include "engine/text.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class auth_scene final : public scene {
  public:
    auth_scene();
    ~auth_scene() override;

  protected:
    void update() override;

  private:
    enum class mode { login, signup, leaderboard };

    mode mode_ = mode::login;
    int field_ = 0;
    bool pending_ = false;
    std::unique_ptr<multi_shoot_client> client_;
    std::string username_;
    std::string password_;
    std::string confirmation_;
    std::string status_;
    std::uint32_t leaderboard_page_ = 0;
    bool has_next_page_ = false;
    bool leaderboard_loaded_ = false;
    std::vector<std::string> leaderboard_rows_;
    text* mode_text_ = nullptr;
    text* username_text_ = nullptr;
    text* password_text_ = nullptr;
    text* confirmation_text_ = nullptr;
    text* status_text_ = nullptr;
    text* help_text_ = nullptr;
    std::vector<text*> leaderboard_texts_;

    void connect();
    void submit();
    void request_leaderboard(std::uint32_t page);
    void handle_response();
    void refresh();
    std::string& current_value();
    text* make_text(int size, float y);
};
