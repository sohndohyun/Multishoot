#pragma once
#include "controllers/game_controller.hpp"
#include "controllers/multi_shoot_client.hpp"

#include <memory>

class multi_controller final : public game_controller {
  protected:
    void update() override;

  public:
    explicit multi_controller(std::unique_ptr<multi_shoot_client> tool);
    ~multi_controller() override = default;

    void change_direction(dr::vector2 direction) override;
    void shoot() override;
    bool is_working() override {
        return tool_ && tool_->is_working();
    }

  private:
    std::unique_ptr<multi_shoot_client> tool_;
};
