#pragma once

#include "morphos/cell.hpp"
#include "morphos/world.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace morphos
{

  struct SimulationConfig
  {
    // World dimensions
    float world_width = 100.0f;
    float world_height = 100.0f;

    // Resource grid
    std::size_t grid_width = 128;
    std::size_t grid_height = 128;

    // Environment
    float initial_resource_capacity = 100.0f;
    float resource_regeneration_rate = 0.01f;

    // Cell parameters
    float initial_cell_energy = 10.0f;
    float absorption_rate = 2.0f;
    float metabolism_rate = 1.0f;

    // Movement
    float movement_speed = 1.0f;
    float movement_cost = 0.05f;

    // Reproducibility
    std::uint32_t seed = 42;
  };

  class Simulation
  {
  public:
    explicit Simulation(const SimulationConfig &config);

    // Backward-compatible constructor.
    Simulation(
        float width,
        float height,
        std::uint32_t seed = 42);

    void initialize(std::size_t population);
    void step(float dt);

    std::size_t population_size() const;
    std::size_t living_population() const;

    std::uint64_t timestep() const;

    const std::vector<Cell> &cells() const;
    const World &world() const;

    const SimulationConfig &config() const;

  private:
    SimulationConfig config_;

    World world_;
    std::vector<Cell> cells_;

    std::uint64_t timestep_ = 0;
  };

}
