#pragma once
#include "controllers/game_controller.hpp"
#include "controllers/multi_shoot_client.hpp"

class multi_controller final : public game_controller {
  protected:
    void update() override;

  public:
    multi_controller();
    ~multi_controller() override;

    void change_direction(dr::vector2 dir, Uint32 id) override;
    void shoot(Uint32 id) override;
    bool is_working() override {
        return work_ && tool_.is_working();
    }

  private:
    multi_shoot_client tool_;
    bool work_;
};
