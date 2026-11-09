#include "morphos/cell.hpp"
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
        absorption_rate_(absorption_rate),
        metabolism_rate_(metabolism_rate),
        movement_speed_(movement_speed),
        movement_cost_(movement_cost),
        alive_(initial_energy > 0.0f)
  {
    if (!std::isfinite(x) ||
        !std::isfinite(y) ||
        !std::isfinite(initial_energy) ||
        !std::isfinite(absorption_rate) ||
        !std::isfinite(metabolism_rate) ||
        !std::isfinite(movement_speed) ||
        !std::isfinite(movement_cost) ||
        initial_energy < 0.0f ||
        absorption_rate < 0.0f ||
        metabolism_rate < 0.0f ||
        movement_speed < 0.0f ||
        movement_cost < 0.0f)
    {
      throw std::invalid_argument(
          "Invalid cell parameters");
    }
  }

  void Cell::move(const World &world, float dt)
  {
    vx_ = 0.0f;
    vy_ = 0.0f;

    if (movement_speed_ == 0.0f)
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

    float actual_speed = movement_speed_;

    // Restrict movement according to available energy.
    if (movement_cost_ > 0.0f)
    {
      const float maximum_affordable_speed =
          std::sqrt(
              available_energy /
              (movement_cost_ * dt));

      actual_speed = std::min(
          movement_speed_,
          maximum_affordable_speed);
    }

    if (actual_speed <= 0.0f)
      return;

    // Normalize the gradient direction.
    vx_ = actual_speed * gradient.dx / magnitude;
    vy_ = actual_speed * gradient.dy / magnitude;

    // Accumulate actual travel distance before periodic wrapping.
    distance_traveled_ += std::hypot(vx_, vy_) * dt;

    // Apply periodic world boundaries.
    x_ = wrap_position(
        x_ + vx_ * dt, world.width());

    y_ = wrap_position(
        y_ + vy_ * dt, world.height());
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

    // Sense and move.
    move(world, dt);

    // Absorb resources at the new position.
    const float absorbed = world.consume_resource(
        x_, y_, absorption_rate_ * dt);

    // Base metabolic expenditure.
    const float metabolic_cost =
        metabolism_rate_ * dt;

    // Movement expenditure.
    const float speed_squared =
        vx_ * vx_ + vy_ * vy_;

    const float locomotion_cost =
        movement_cost_ * speed_squared * dt;

    energy_ = std::max(
        0.0f,
        energy_ + absorbed - metabolic_cost - locomotion_cost);

    ++age_;

    if (energy_ <= 0.0f)
    {
      alive_ = false;
      vx_ = 0.0f;
      vy_ = 0.0f;
    }
  }

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
