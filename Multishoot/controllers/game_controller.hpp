#pragma once

#include "containers/mpsc_channel.hpp"
#include "engine/object.hpp"
#include "math/vector.hpp"
#include "multishoot/protocol/game.pb.h"

#include <optional>

class game_controller : public object {
  protected:
    dr::mpsc_channel<multishoot::protocol::ServerPacket> response_channel_;

    void enqueue(multishoot::protocol::ServerPacket packet);

  public:
    virtual void change_direction(dr::vector2 direction) = 0;
    virtual void shoot() = 0;
    virtual bool is_working() {
        return true;
    }

    std::optional<multishoot::protocol::ServerPacket> pop();
    ~game_controller() override = default;
};
