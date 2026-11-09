
#include "morphos/cell.hpp"
#include "morphos/world.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace morphos
{

  Cell::Cell(float x, float y,
             float initial_energy,
             float absorption_rate,
             float metabolism_rate)
      : x_(x),
        y_(y),
        energy_(initial_energy),
        absorption_rate_(absorption_rate),
        metabolism_rate_(metabolism_rate),
        alive_(initial_energy > 0.0f)
  {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(initial_energy) ||
        !std::isfinite(absorption_rate) ||
        !std::isfinite(metabolism_rate) ||
        initial_energy < 0.0f ||
        absorption_rate < 0.0f ||
        metabolism_rate < 0.0f)
    {
      throw std::invalid_argument(
          "Invalid cell parameters");
    }
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

    const float requested = absorption_rate_ * dt;

    const float absorbed = world.consume_resource(
        x_, y_, requested);

    const float metabolic_cost = metabolism_rate_ * dt;

    energy_ = std::max(
        0.0f,
        energy_ + absorbed - metabolic_cost);

    ++age_;

    if (energy_ <= 0.0f)
      alive_ = false;
  }

  float Cell::x() const
  {
    return x_;
  }

  float Cell::y() const
  {
    return y_;
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

}
