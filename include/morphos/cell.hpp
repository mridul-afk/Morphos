#pragma once

#include "morphos/genome.hpp"

#include <cstdint>
#include <optional>
#include <random>

namespace morphos
{

  class World;

  class Cell
  {
  public:
    Cell(float x, float y,
         float initial_energy = 10.0f,
         float absorption_rate = 2.0f,
         float metabolism_rate = 1.0f,
         float movement_speed = 1.0f,
         float movement_cost = 0.05f);

    void update(World &world, float dt);

    float x() const;
    float y() const;

    float vx() const;
    float vy() const;

    float energy() const;

    float distance_traveled() const;

    bool is_alive() const;
    std::uint64_t age() const;

    [[nodiscard]] const Genome &genome() const noexcept;

    void set_genome(const Genome &genome);

    // Attempt asexual reproduction.
    // Returns an offspring when the parent has sufficient
    // energy, or std::nullopt otherwise.
    // A successful reproduction divides the parent's
    // energy equally between parent and offspring.
    [[nodiscard]] std::optional<Cell> reproduce(
        std::mt19937 &rng,
        float energy_threshold,
        float mutation_probability = 0.1f,
        float mutation_strength = 0.1f);

    [[nodiscard]] std::uint64_t id() const noexcept;
    [[nodiscard]] std::uint64_t parent_id() const noexcept;
    [[nodiscard]] std::uint64_t generation() const noexcept;

    void set_lineage(
        std::uint64_t id,
        std::uint64_t parent_id,
        std::uint64_t generation);

  private:
    void move(const World &world, float dt);

    float x_;
    float y_;

    float vx_ = 0.0f;
    float vy_ = 0.0f;

    float energy_;

    float distance_traveled_ = 0.0f;

    float metabolism_rate_;

    float movement_cost_;

    bool alive_ = true;
    std::uint64_t age_ = 0;

    Genome genome_;

    std::uint64_t id_ = 0;
    std::uint64_t parent_id_ = 0;
    std::uint64_t generation_ = 0;
  };

}
