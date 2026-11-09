#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace morphos
{

  struct ResourceGradient
  {
    float dx = 0.0f;
    float dy = 0.0f;
  };

  class World
  {
  public:
    World(float width, float height,
          std::size_t grid_width = 128,
          std::size_t grid_height = 128,
          std::uint32_t seed = 42);

    float width() const;
    float height() const;

    void initialize_resources(float max_energy);

    float resource_at(float x, float y) const;

    float consume_resource(float x, float y,
                           float requested);

    void regenerate_resources(float rate, float dt);

    ResourceGradient resource_gradient(float x, float y) const;
    float cell_width() const;
    float cell_height() const;

  private:
    std::size_t index(float x, float y) const;

    float width_;
    float height_;

    std::size_t grid_width_;
    std::size_t grid_height_;

    std::vector<float> resources_;
    float resource_capacity_ = 0.0f;

    std::mt19937 rng_;
  };
}
