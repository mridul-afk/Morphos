
#include "morphos/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace morphos
{

  Simulation::Simulation(float width, float height,
                         std::uint32_t seed)
      : world_(width, height, 128, 128, seed),
        seed_(seed)
  {
  }

  void Simulation::initialize(std::size_t population)
  {
    cells_.clear();
    cells_.reserve(population);

    world_.initialize_resources(100.0f);

    std::mt19937 rng(seed_);

    std::uniform_real_distribution<float> x_dist(
        0.0f, world_.width());

    std::uniform_real_distribution<float> y_dist(
        0.0f, world_.height());

    for (std::size_t i = 0; i < population; ++i)
    {
      cells_.emplace_back(
          x_dist(rng),
          y_dist(rng),
          10.0f,
          2.0f,
          1.0f);
    }

    timestep_ = 0;
  }

  void Simulation::step(float dt)
  {
    if (!std::isfinite(dt) || dt <= 0.0f)
      throw std::invalid_argument("dt must be positive");

    for (Cell &cell : cells_)
    {
      cell.update(world_, dt);
    }

    world_.regenerate_resources(0.01f, dt);

    ++timestep_;
  }

  std::size_t Simulation::population_size() const
  {
    return cells_.size();
  }

  std::size_t Simulation::living_population() const
  {
    return static_cast<std::size_t>(
        std::count_if(
            cells_.begin(),
            cells_.end(),
            [](const Cell &cell)
            {
              return cell.is_alive();
            }));
  }

  std::uint64_t Simulation::timestep() const
  {
    return timestep_;
  }

  const std::vector<Cell> &Simulation::cells() const
  {
    return cells_;
  }

  const World &Simulation::world() const
  {
    return world_;
  }

}
