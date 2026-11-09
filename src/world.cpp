#include "morphos/world.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace morphos
{

  World::World(float width, float height,
               std::size_t grid_width,
               std::size_t grid_height,
               std::uint32_t seed)
      : width_(width),
        height_(height),
        grid_width_(grid_width),
        grid_height_(grid_height),
        resources_(),
        rng_(seed)
  {
    if (!std::isfinite(width_) ||
        !std::isfinite(height_) ||
        width_ <= 0.0f ||
        height_ <= 0.0f ||
        grid_width_ == 0 ||
        grid_height_ == 0)
    {
      throw std::invalid_argument(
          "World dimensions must be positive and finite");
    }

    if (grid_width_ >
        resources_.max_size() / grid_height_)
    {
      throw std::length_error(
          "Resource grid dimensions are too large");
    }

    resources_.resize(
        grid_width_ * grid_height_, 0.0f);
  }

  float World::width() const
  {
    return width_;
  }

  float World::height() const
  {
    return height_;
  }

  std::size_t World::index(float x, float y) const
  {
    if (!std::isfinite(x) || !std::isfinite(y))
    {
      throw std::invalid_argument(
          "Coordinates must be finite");
    }

    // Periodic boundary conditions:
    // Leaving one edge enters through the opposite edge.
    x = std::fmod(x, width_);
    y = std::fmod(y, height_);

    if (x < 0.0f)
      x += width_;

    if (y < 0.0f)
      y += height_;

    const std::size_t gx = std::min(
        static_cast<std::size_t>(
            (x / width_) * grid_width_),
        grid_width_ - 1);

    const std::size_t gy = std::min(
        static_cast<std::size_t>(
            (y / height_) * grid_height_),
        grid_height_ - 1);

    return gy * grid_width_ + gx;
  }

  void World::initialize_resources(float max_energy)
  {
    if (!std::isfinite(max_energy) ||
        max_energy < 0.0f)
    {
      throw std::invalid_argument(
          "Resource capacity must be nonnegative and finite");
    }

    resource_capacity_ = max_energy;

    std::uniform_real_distribution<float> distribution(
        0.0f, max_energy);

    for (float &resource : resources_)
    {
      resource = distribution(rng_);
    }
  }

  float World::resource_at(float x, float y) const
  {
    return resources_[index(x, y)];
  }

  float World::consume_resource(float x, float y,
                                float requested)
  {
    if (!std::isfinite(requested) ||
        requested < 0.0f)
    {
      throw std::invalid_argument(
          "Requested energy must be nonnegative and finite");
    }

    float &available = resources_[index(x, y)];

    const float consumed = std::min(
        available, requested);

    available -= consumed;

    return consumed;
  }

  void World::regenerate_resources(float rate, float dt)
  {
    if (!std::isfinite(rate) ||
        !std::isfinite(dt) ||
        rate < 0.0f ||
        dt < 0.0f)
    {
      throw std::invalid_argument(
          "Regeneration rate and dt must be nonnegative and finite");
    }

    const float replenishment = rate * dt;

    for (float &resource : resources_)
    {
      resource = std::min(
          resource_capacity_,
          resource + replenishment);
    }
  }

  float World::cell_width() const
  {
    return width_ / static_cast<float>(grid_width_);
  }

  float World::cell_height() const
  {
    return height_ / static_cast<float>(grid_height_);
  }

  ResourceGradient World::resource_gradient(
      float x, float y) const
  {
    const float hx = cell_width();
    const float hy = cell_height();

    const float right = resource_at(x + hx, y);
    const float left = resource_at(x - hx, y);

    const float up = resource_at(x, y + hy);
    const float down = resource_at(x, y - hy);

    return {
        (right - left) / (2.0f * hx),
        (up - down) / (2.0f * hy)};
  }

}
