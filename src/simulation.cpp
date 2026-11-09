#include "morphos/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace morphos
{

  namespace
  {

    void validate_config(const SimulationConfig &c)
    {
      auto valid_nonnegative = [](float x)
      {
        return std::isfinite(x) && x >= 0.0f;
      };

      if (!std::isfinite(c.world_width) ||
          !std::isfinite(c.world_height) ||
          c.world_width <= 0.0f ||
          c.world_height <= 0.0f ||
          c.grid_width == 0 ||
          c.grid_height == 0 ||
          !valid_nonnegative(c.initial_resource_capacity) ||
          !valid_nonnegative(c.resource_regeneration_rate) ||
          !valid_nonnegative(c.initial_cell_energy) ||
          !valid_nonnegative(c.absorption_rate) ||
          !valid_nonnegative(c.metabolism_rate) ||
          !valid_nonnegative(c.movement_speed) ||
          !valid_nonnegative(c.movement_cost))
      {
        throw std::invalid_argument(
            "Invalid SimulationConfig parameters");
      }
    }

    SimulationConfig legacy_config(
        float width,
        float height,
        std::uint32_t seed)
    {
      SimulationConfig config;
      config.world_width = width;
      config.world_height = height;
      config.seed = seed;

      return config;
    }

  } // anonymous namespace

  Simulation::Simulation(const SimulationConfig &config)
      : config_(config),
        world_(
            config.world_width,
            config.world_height,
            config.grid_width,
            config.grid_height,
            config.seed)
  {
    validate_config(config_);
  }

  Simulation::Simulation(
      float width,
      float height,
      std::uint32_t seed)
      : Simulation(legacy_config(width, height, seed))
  {
  }

  void Simulation::initialize(std::size_t population)
  {
    cells_.clear();
    cells_.reserve(population);

    world_.initialize_resources(
        config_.initial_resource_capacity);

    world_.smooth_resources(
        config_.resource_smoothing_passes);

    std::mt19937 rng(config_.seed);

    std::uniform_real_distribution<float> x_dist(
        0.0f, config_.world_width);

    std::uniform_real_distribution<float> y_dist(
        0.0f, config_.world_height);

    for (std::size_t i = 0; i < population; ++i)
    {
      cells_.emplace_back(
          x_dist(rng),
          y_dist(rng),
          config_.initial_cell_energy,
          config_.absorption_rate,
          config_.metabolism_rate,
          config_.movement_speed,
          config_.movement_cost);
    }

    timestep_ = 0;
  }

  void Simulation::step(float dt)
  {
    if (!std::isfinite(dt) || dt <= 0.0f)
    {
      throw std::invalid_argument(
          "Simulation timestep must be positive");
    }

    for (Cell &cell : cells_)
    {
      cell.update(world_, dt);
    }

    world_.regenerate_resources(
        config_.resource_regeneration_rate, dt);

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

  const SimulationConfig &Simulation::config() const
  {
    return config_;
  }

}
