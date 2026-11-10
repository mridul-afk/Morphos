#pragma once

#include <random>

namespace morphos
{

  struct Genome
  {
    // Default gene values, not hardcoded limits.
    float movement_speed = 1.0f;
    float absorption_rate = 2.0f;

    // Validates physiological gene values.
    void validate() const;

    // Returns a mutated copy of this genome.
    [[nodiscard]] Genome mutated(
        std::mt19937 &rng,
        float mutation_probability = 0.1f,
        float mutation_strength = 0.1f) const;
  };

}
