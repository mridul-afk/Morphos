#include "morphos/cell.hpp"
#include "morphos/genome.hpp"
#include "morphos/world.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace morphos
{

  namespace
  {

    float wrap_position(float value, float extent)
    {
      value = std::fmod(value, extent);

      if (value < 0.0f)
        value += extent;

      return value;
    }

  }

  Cell::Cell(float x, float y,
             float initial_energy,
             float absorption_rate,
             float metabolism_rate,
             float movement_speed,
             float movement_cost)
      : x_(x),
        y_(y),
        energy_(initial_energy),
        genome_{movement_speed, absorption_rate},
        metabolism_rate_(metabolism_rate),
        movement_cost_(movement_cost),
        alive_(initial_energy > 0.0f)
  {
    if (!std::isfinite(x) ||
        !std::isfinite(y) ||
        !std::isfinite(initial_energy) ||
        !std::isfinite(metabolism_rate) ||
        !std::isfinite(movement_cost) ||
        initial_energy < 0.0f ||
        metabolism_rate < 0.0f ||
        movement_cost < 0.0f)
    {
      throw std::invalid_argument(
          "Invalid cell parameters");
    }

    // Validate the two heritable traits.
    genome_.validate();
  }

  void Cell::move(const World &world, float dt)
  {
    vx_ = 0.0f;
    vy_ = 0.0f;

    // Movement speed is now genetically encoded.
    const float movement_speed =
        genome_.movement_speed;

    if (movement_speed == 0.0f)
      return;

    const ResourceGradient gradient =
        world.resource_gradient(x_, y_);

    const float magnitude = std::hypot(
        gradient.dx, gradient.dy);

    if (magnitude <= 1e-6f)
      return;

    // Reserve energy for metabolism first.
    const float metabolic_reserve =
        metabolism_rate_ * dt;

    const float available_energy = std::max(
        0.0f, energy_ - metabolic_reserve);

    float actual_speed = movement_speed;

    // Restrict movement according to available energy.
    if (movement_cost_ > 0.0f)
    {
      const float maximum_affordable_speed =
          std::sqrt(
              available_energy /
              (movement_cost_ * dt));

      actual_speed = std::min(
          movement_speed,
          maximum_affordable_speed);
    }

    if (actual_speed <= 0.0f)
      return;

    // Normalize the gradient direction.
    vx_ = actual_speed * gradient.dx / magnitude;
    vy_ = actual_speed * gradient.dy / magnitude;

    // Accumulate actual travel distance before periodic wrapping.
    distance_traveled_ +=
        std::hypot(vx_, vy_) * dt;

    // Apply periodic world boundaries.
    x_ = wrap_position(
        x_ + vx_ * dt,
        world.width());

    y_ = wrap_position(
        y_ + vy_ * dt,
        world.height());
  }

  void Cell::update(World &world, float dt)
  {
    if (!std::isfinite(dt) || dt < 0.0f)
    {
      throw std::invalid_argument(
          "Invalid simulation timestep");
    }

    if (!alive_ || dt == 0.0f)
      return;

    // Sense and move using genome-defined movement speed.
    move(world, dt);

    // Resource absorption is now genetically encoded.
    const float absorbed =
        world.consume_resource(
            x_,
            y_,
            genome_.absorption_rate * dt);

    // Base metabolic expenditure.
    const float metabolic_cost =
        metabolism_rate_ * dt;

    // Movement expenditure.
    const float speed_squared =
        vx_ * vx_ + vy_ * vy_;

    const float locomotion_cost =
        movement_cost_ * speed_squared * dt;

    // Update cell energy.
    energy_ = std::max(
        0.0f,
        energy_ + absorbed -
            metabolic_cost -
            locomotion_cost);

    ++age_;

    // Cell dies when its energy is exhausted.
    if (energy_ <= 0.0f)
    {
      alive_ = false;
      vx_ = 0.0f;
      vy_ = 0.0f;
    }
  }
  // Genome access and modification
  const Genome &Cell::genome() const noexcept
  {
    return genome_;
  }

  void Cell::set_genome(const Genome &genome)
  {
    // Validate before assignment so invalid genomes
    // cannot modify the cell.
    genome.validate();

    genome_ = genome;
  }

  std::optional<Cell> Cell::reproduce(
      std::mt19937 &rng,
      float energy_threshold,
      float mutation_probability,
      float mutation_strength)
  {
    // Reproduction threshold must be finite and positive.
    if (!std::isfinite(energy_threshold) ||
        energy_threshold <= 0.0f)
    {
      throw std::invalid_argument(
          "Reproduction threshold must be positive and finite");
    }

    // Validate mutation settings even if reproduction
    // ultimately cannot take place.
    if (!std::isfinite(mutation_probability) ||
        mutation_probability < 0.0f ||
        mutation_probability > 1.0f ||
        !std::isfinite(mutation_strength) ||
        mutation_strength < 0.0f)
    {
      throw std::invalid_argument(
          "Invalid reproduction mutation parameters");
    }

    // Dead or under-energized cells cannot reproduce.
    if (!alive_ || energy_ < energy_threshold)
    {
      return std::nullopt;
    }

    // Produce a potentially mutated copy of the genome.
    // Do this before modifying parent energy.
    const Genome offspring_genome = genome_.mutated(
        rng,
        mutation_probability,
        mutation_strength);

    const float offspring_energy = energy_ * 0.5f;

    // The offspring begins at the parent's location.
    // Spatial displacement can be introduced later.
    Cell offspring(
        x_,
        y_,
        offspring_energy,
        offspring_genome.absorption_rate,
        metabolism_rate_,
        offspring_genome.movement_speed,
        movement_cost_);

    // Transfer energy from parent to offspring.
    // Subtraction preserves the total more accurately
    // than assigning both energies independently.
    energy_ -= offspring_energy;

    return offspring;
  }

  // Cell state accessors
  float Cell::x() const
  {
    return x_;
  }

  float Cell::y() const
  {
    return y_;
  }

  float Cell::vx() const
  {
    return vx_;
  }

  float Cell::vy() const
  {
    return vy_;
  }

  float Cell::energy() const
  {
    return energy_;
  }

  bool Cell::is_alive() const
  {
    return alive_;
  }

  std::uint64_t Cell::age() const
  {
    return age_;
  }

  float Cell::distance_traveled() const
  {
    return distance_traveled_;
  }

}
