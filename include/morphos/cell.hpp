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
         float metabolism_rate = 1.0f);

    void update(World &world, float dt);

    float x() const;
    float y() const;
    float energy() const;

    bool is_alive() const;
    std::uint64_t age() const;

  private:
    float x_;
    float y_;
    float energy_;

    float absorption_rate_;
    float metabolism_rate_;

    bool alive_ = true;
    std::uint64_t age_ = 0;
  };
}
