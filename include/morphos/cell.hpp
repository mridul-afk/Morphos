#pragma once

#include <cstdint>

namespace morphos
{

  class World;

  class Cell
  {
  public:
    Cell(float x, float y,
         float initial_energy = 10.0f,
         float absorption_rate = 2.0f,
         float metabolism_rate = 1.0f,
         float movement_speed = 1.0f,
         float movement_cost = 0.05f);

    void update(World &world, float dt);

    float x() const;
    float y() const;

    float vx() const;
    float vy() const;

    float energy() const;

    float distance_traveled() const;

    bool is_alive() const;
    std::uint64_t age() const;

  private:
    void move(const World &world, float dt);

    float x_;
    float y_;

    float vx_ = 0.0f;
    float vy_ = 0.0f;

    float energy_;

    float distance_traveled_ = 0.0f;

    float absorption_rate_;
    float metabolism_rate_;

    float movement_speed_;
    float movement_cost_;

    bool alive_ = true;
    std::uint64_t age_ = 0;
  };

}
