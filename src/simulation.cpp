
#include "morphos/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

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

  // Constructors

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

  // Lineage registration

  void Simulation::register_birth(
      Cell &cell,
      std::uint64_t parent_id,
      std::uint64_t generation,
      std::uint64_t birth_timestep)
  {
    if (next_cell_id_ ==
        std::numeric_limits<std::uint64_t>::max())
    {
      throw std::overflow_error(
          "Cell ID space exhausted");
    }

    if (cell.id() != 0)
    {
      throw std::logic_error(
          "Cannot register an already identified cell");
    }

    if ((parent_id == 0 && generation != 0) ||
        (parent_id != 0 && generation == 0))
    {
      throw std::invalid_argument(
          "Invalid parent and generation relationship");
    }

    if (parent_id != 0 &&
        lineage_index_.find(parent_id) ==
            lineage_index_.end())
    {
      throw std::logic_error(
          "Parent does not exist in lineage history");
    }

    const std::uint64_t id = next_cell_id_;

    LineageRecord record;

    record.id = id;
    record.parent_id = parent_id;
    record.generation = generation;

    record.birth_timestep = birth_timestep;
    record.death_timestep = 0;

    record.alive = cell.is_alive();
    record.birth_genome = cell.genome();

    lineage_records_.push_back(record);

    try
    {
      const auto result = lineage_index_.emplace(
          id,
          lineage_records_.size() - 1);

      if (!result.second)
      {
        throw std::logic_error(
            "Duplicate lineage identifier");
      }
    }
    catch (...)
    {
      lineage_records_.pop_back();
      throw;
    }

    cell.set_lineage(
        id,
        parent_id,
        generation);

    ++next_cell_id_;
  }

  void Simulation::register_death(
      std::uint64_t cell_id,
      std::uint64_t death_timestep)
  {
    const auto it =
        lineage_index_.find(cell_id);

    if (it == lineage_index_.end())
    {
      throw std::logic_error(
          "Death recorded for unknown organism");
    }

    LineageRecord &record =
        lineage_records_[it->second];

    if (!record.alive)
    {
      return;
    }

    record.alive = false;
    record.death_timestep = death_timestep;
  }

  const std::vector<LineageRecord> &
  Simulation::lineage_records() const noexcept
  {
    return lineage_records_;
  }

  const LineageRecord *
  Simulation::find_lineage(
      std::uint64_t id) const noexcept
  {
    const auto it =
        lineage_index_.find(id);

    if (it == lineage_index_.end())
    {
      return nullptr;
    }

    return &lineage_records_[it->second];
  }

  // Initialization

  void Simulation::initialize(std::size_t population)
  {
    if (config_.enable_reproduction &&
        population > config_.max_population)
    {
      throw std::invalid_argument(
          "Initial population exceeds max_population");
    }

    // Reset population.
    cells_.clear();
    cells_.reserve(population);

    // Reset lineage history.
    next_cell_id_ = 1;

    lineage_records_.clear();
    lineage_index_.clear();

    lineage_records_.reserve(population);
    lineage_index_.reserve(population);

    // Initialize environmental resources.
    world_.initialize_resources(
        config_.initial_resource_capacity);

    world_.smooth_resources(
        config_.resource_smoothing_passes);

    // Preserve original initialization RNG behavior.
    std::mt19937 rng(config_.seed);

    std::uniform_real_distribution<float> x_dist(
        0.0f,
        config_.world_width);

    std::uniform_real_distribution<float> y_dist(
        0.0f,
        config_.world_height);

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

      Cell &founder = cells_.back();

      register_birth(
          founder,
          0, // No parent
          0, // Founding generation
          0  // Initial timestep
      );
    }

    // Reset reproduction RNG.
    reproduction_rng_.seed(config_.seed);

    // Reset simulation statistics.
    timestep_ = 0;
    total_births_ = 0;
    total_deaths_ = 0;
  }

  // Simulation step

  void Simulation::step(float dt)
  {
    if (!std::isfinite(dt) || dt <= 0.0f)
    {
      throw std::invalid_argument(
          "Simulation timestep must be positive");
    }

    // Phase 1: Update existing cells

    for (Cell &cell : cells_)
    {
      cell.update(world_, dt);
    }

    // Phase 2: Record deaths
    //
    // Deaths must be recorded before dead cells are removed.
    //
    // This works with reproduction enabled or disabled.
    // Repeated observations of the same dead cell do not
    // create duplicate death events.

    for (const Cell &cell : cells_)
    {
      if (!cell.is_alive())
      {
        register_death(
            cell.id(),
            timestep_ + 1);
      }
    }

    // Phase 3: Population dynamics

    if (config_.enable_reproduction)
    {
      // Remove dead organisms from the active population.
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

      // Phase 4: Reproduction
      //
      // Newborns are collected separately.
      // They cannot reproduce during their birth timestep.

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

        if (!child.has_value())
        {
          continue;
        }

        // Ensure generation cannot overflow.
        if (parent.generation() ==
            std::numeric_limits<std::uint64_t>::max())
        {
          throw std::overflow_error(
              "Cell generation limit exceeded");
        }

        // Assign the newborn its unique identity.
        register_birth(
            *child,
            parent.id(),
            parent.generation() + 1,
            timestep_ + 1);

        offspring.push_back(
            std::move(*child));
      }

      // Phase 5: Insert newborns

      cells_.insert(
          cells_.end(),
          std::make_move_iterator(offspring.begin()),
          std::make_move_iterator(offspring.end()));

      total_births_ +=
          static_cast<std::uint64_t>(
              offspring.size());
    }

    // Phase 6: Regenerate environmental resources

    world_.regenerate_resources(
        config_.resource_regeneration_rate,
        dt);

    // Advance simulation clock.
    ++timestep_;
  }

  // Population statistics

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

  // Accessors

  const std::vector<Cell> &
  Simulation::cells() const
  {
    return cells_;
  }

  const World &
  Simulation::world() const
  {
    return world_;
  }

  const SimulationConfig &
  Simulation::config() const
  {
    return config_;
  }

}
