#pragma once

#include "morphos/genome.hpp"

#include <cstdint>

namespace morphos
{
  struct LineageRecord
  {
    std::uint64_t id = 0;
    std::uint64_t parent_id = 0;
    std::uint64_t generation = 0;

    std::uint64_t birth_timestep = 0;
    std::uint64_t death_timestep = 0;

    bool alive = true;

    // Genome at birth, retained after death.
    Genome birth_genome;
  };
}
