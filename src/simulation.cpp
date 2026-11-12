#include "morphos/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>
#include <iterator>

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
          !valid_nonnegative(c.movement_cost) ||
          !std::isfinite(c.reproduction_threshold) ||
          c.reproduction_threshold <= 0.0f ||
          !std::isfinite(c.mutation_probability) ||
          c.mutation_probability < 0.0f ||
          c.mutation_probability > 1.0f ||
          !valid_nonnegative(c.mutation_strength) ||
          c.max_population == 0)
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
            config.seed),
        reproduction_rng_(config.seed)
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
    if (config_.enable_reproduction &&
        population > config_.max_population)
    {
      throw std::invalid_argument(
          "Initial population exceeds max_population");
    }

    cells_.clear();
    cells_.reserve(population);

    world_.initialize_resources(
        config_.initial_resource_capacity);

    world_.smooth_resources(
        config_.resource_smoothing_passes);

    // Keep the original initialization RNG behavior.
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

    // Reset reproduction randomness and statistics.
    reproduction_rng_.seed(config_.seed);

    timestep_ = 0;
    total_births_ = 0;
    total_deaths_ = 0;
  }

  void Simulation::step(float dt)
  {
    if (!std::isfinite(dt) || dt <= 0.0f)
    {
      throw std::invalid_argument(
          "Simulation timestep must be positive");
    }

    // Phase 1: Update all existing cells.
    //
    // No newborns are inserted during this loop.
    for (Cell &cell : cells_)
    {
      cell.update(world_, dt);
    }

    if (config_.enable_reproduction)
    {
      // Phase 2: Remove dead cells.
      //
      // Store the number removed so deaths can be
      // counted even after cells leave the vector.
      const std::size_t before = cells_.size();

      cells_.erase(
          std::remove_if(
              cells_.begin(),
              cells_.end(),
              [](const Cell &cell)
              {
                return !cell.is_alive();
              }),
          cells_.end());

      total_deaths_ +=
          static_cast<std::uint64_t>(
              before - cells_.size());

      // Phase 3: Reproduction.
      //
      // Keep offspring separate until every existing
      // parent has been considered.
      //
      // This prevents newborns from reproducing
      // during their birth timestep.
      std::vector<Cell> offspring;

      const std::size_t parent_count = cells_.size();

      const std::size_t available_slots =
          config_.max_population - parent_count;

      offspring.reserve(
          std::min(parent_count, available_slots));

      for (std::size_t i = 0; i < parent_count; ++i)
      {
        if (offspring.size() >= available_slots)
        {
          break;
        }

        Cell &parent = cells_[i];

        if (parent.energy() <
            config_.reproduction_threshold)
        {
          continue;
        }

        auto child = parent.reproduce(
            reproduction_rng_,
            config_.reproduction_threshold,
            config_.mutation_probability,
            config_.mutation_strength);

        if (child.has_value())
        {
          offspring.push_back(
              std::move(*child));
        }
      }

      // Phase 4: Insert newborns.
      //
      // Existing parents have finished their update.
      // Newborns begin updating on the next timestep.
      total_births_ +=
          static_cast<std::uint64_t>(
              offspring.size());

      cells_.insert(
          cells_.end(),
          std::make_move_iterator(offspring.begin()),
          std::make_move_iterator(offspring.end()));
    }

    // Phase 5: Regenerate environmental resources.
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

  std::uint64_t Simulation::total_births() const
  {
    return total_births_;
  }

  std::uint64_t Simulation::total_deaths() const
  {
    return total_deaths_;
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
