
#pragma once

#include "morphos/cell.hpp"
#include "morphos/world.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace morphos
{

  class Simulation
  {
  public:
    Simulation(float width, float height,
               std::uint32_t seed = 42);

    void initialize(std::size_t population);

    void step(float dt);

    std::size_t population_size() const;
    std::size_t living_population() const;

    std::uint64_t timestep() const;

    const std::vector<Cell> &cells() const;
    const World &world() const;

  private:
    World world_;
    std::vector<Cell> cells_;

    std::uint64_t timestep_ = 0;
    std::uint32_t seed_;
  };

}
