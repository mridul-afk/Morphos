
#include <gtest/gtest.h>

#include "morphos/genome.hpp"
#include "morphos/world.hpp"
#include "morphos/cell.hpp"

#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>

using morphos::Cell;
using morphos::Genome;
using morphos::World;

// ============================================================
// GENOME UNIT TESTS
// ============================================================

TEST(GenomeTest, DefaultGenomeIsValid)
{
    Genome genome;

    EXPECT_NO_THROW(genome.validate());
    EXPECT_FLOAT_EQ(genome.movement_speed, 1.0f);
    EXPECT_FLOAT_EQ(genome.absorption_rate, 2.0f);
}

TEST(GenomeTest, InvalidGenesAreRejected)
{
    Genome genome;

    genome.movement_speed = -1.0f;

    EXPECT_THROW(
        genome.validate(),
        std::invalid_argument);

    genome.movement_speed = 1.0f;
    genome.absorption_rate = -2.0f;

    EXPECT_THROW(
        genome.validate(),
        std::invalid_argument);
}

TEST(GenomeTest, ZeroMutationProbabilityPreservesGenome)
{
    std::mt19937 rng(42);

    Genome parent{0.25f, 2.0f};

    Genome child = parent.mutated(
        rng,
        0.0f,
        0.5f);

    EXPECT_FLOAT_EQ(
        child.movement_speed,
        parent.movement_speed);

    EXPECT_FLOAT_EQ(
        child.absorption_rate,
        parent.absorption_rate);
}

TEST(GenomeTest, MutationsRemainValid)
{
    std::mt19937 rng(42);

    Genome genome{0.25f, 2.0f};

    for (int i = 0; i < 10000; ++i)
    {
        genome = genome.mutated(
            rng,
            1.0f,
            1.0f);

        EXPECT_NO_THROW(genome.validate());
    }
}

TEST(GenomeTest, ParentGenomeRemainsUnchanged)
{
    std::mt19937 rng(42);

    const Genome parent{0.25f, 2.0f};

    Genome child = parent.mutated(
        rng,
        1.0f,
        0.2f);

    EXPECT_FLOAT_EQ(parent.movement_speed, 0.25f);
    EXPECT_FLOAT_EQ(parent.absorption_rate, 2.0f);

    EXPECT_NO_THROW(child.validate());
}

TEST(GenomeTest, MutationIsReproducibleWithSameSeed)
{
    std::mt19937 rng_a(42);
    std::mt19937 rng_b(42);

    const Genome parent{0.25f, 2.0f};

    const Genome child_a = parent.mutated(
        rng_a, 1.0f, 0.2f);

    const Genome child_b = parent.mutated(
        rng_b, 1.0f, 0.2f);

    EXPECT_FLOAT_EQ(
        child_a.movement_speed,
        child_b.movement_speed);

    EXPECT_FLOAT_EQ(
        child_a.absorption_rate,
        child_b.absorption_rate);
}

// ============================================================
// GENOME-TO-BEHAVIOR INTEGRATION TESTS
// ============================================================

// Verify that movement_speed controls actual distance
// traveled under a nonzero resource gradient.
TEST(GenomeIntegrationTest, MovementGeneControlsDistance)
{
    constexpr std::uint32_t seed = 42;

    constexpr float x = 10.5f;
    constexpr float y = 10.5f;
    constexpr float dt = 1.0f;

    // Two independent, identically initialized worlds.
    World world_slow(32.0f, 32.0f, 32, 32, seed);
    World world_fast(32.0f, 32.0f, 32, 32, seed);

    world_slow.initialize_resources(10.0f);
    world_fast.initialize_resources(10.0f);

    // Verify the environments are identical at the
    // initial position.
    EXPECT_FLOAT_EQ(
        world_slow.resource_at(x, y),
        world_fast.resource_at(x, y));

    // Movement requires a nonzero resource gradient.
    const auto gradient =
        world_slow.resource_gradient(x, y);

    ASSERT_GT(
        std::hypot(gradient.dx, gradient.dy),
        1e-6f);

    // Identical cells, except for their genomes.
    Cell slow(
        x, y,
        100.0f, // Initial energy
        0.0f,   // Absorption
        0.0f,   // Metabolism
        0.25f,  // Initial movement speed
        0.0f    // Movement cost
    );

    Cell fast = slow;

    Genome slow_genome = slow.genome();
    Genome fast_genome = fast.genome();

    slow_genome.movement_speed = 0.25f;
    fast_genome.movement_speed = 2.0f;

    slow.set_genome(slow_genome);
    fast.set_genome(fast_genome);

    // Verify gene assignments.
    EXPECT_FLOAT_EQ(
        slow.genome().movement_speed,
        0.25f);

    EXPECT_FLOAT_EQ(
        fast.genome().movement_speed,
        2.0f);

    // Execute one simulation update per cell.
    slow.update(world_slow, dt);
    fast.update(world_fast, dt);

    // With no movement cost, both cells should
    // travel at their genome-defined speeds.
    EXPECT_NEAR(
        slow.distance_traveled(),
        0.25f,
        1e-5f);

    EXPECT_NEAR(
        fast.distance_traveled(),
        2.0f,
        1e-5f);

    EXPECT_GT(
        fast.distance_traveled(),
        slow.distance_traveled());

    EXPECT_TRUE(slow.is_alive());
    EXPECT_TRUE(fast.is_alive());
}

// Verify that absorption_rate controls energy uptake
// and resource consumption.
TEST(GenomeIntegrationTest, AbsorptionGeneControlsEnergy)
{
    constexpr std::uint32_t seed = 42;

    constexpr float x = 10.5f;
    constexpr float y = 10.5f;
    constexpr float dt = 1.0f;

    World world_low(32.0f, 32.0f, 32, 32, seed);
    World world_high(32.0f, 32.0f, 32, 32, seed);

    // Initialize with sufficient resource capacity.
    world_low.initialize_resources(100.0f);
    world_high.initialize_resources(100.0f);

    const float initial_resource =
        world_low.resource_at(x, y);

    // Verify identical initial conditions.
    EXPECT_FLOAT_EQ(
        initial_resource,
        world_high.resource_at(x, y));

    // Both cells must have enough available
    // resources to express their absorption rates.
    ASSERT_GE(initial_resource, 2.0f);

    Cell low(
        x, y,
        10.0f, // Initial energy
        0.5f,  // Initial absorption rate
        0.0f,  // Metabolism
        0.0f,  // Movement speed
        0.0f   // Movement cost
    );

    Cell high = low;

    Genome low_genome = low.genome();
    Genome high_genome = high.genome();

    low_genome.absorption_rate = 0.5f;
    high_genome.absorption_rate = 2.0f;

    low.set_genome(low_genome);
    high.set_genome(high_genome);

    EXPECT_FLOAT_EQ(
        low.genome().absorption_rate,
        0.5f);

    EXPECT_FLOAT_EQ(
        high.genome().absorption_rate,
        2.0f);

    // Stationary cells consume resources at
    // the same location in independent worlds.
    low.update(world_low, dt);
    high.update(world_high, dt);

    // Verify energy gained by each cell.
    EXPECT_NEAR(
        low.energy(),
        10.5f,
        1e-5f);

    EXPECT_NEAR(
        high.energy(),
        12.0f,
        1e-5f);

    EXPECT_GT(
        high.energy(),
        low.energy());

    // Verify conservation of consumed resources.
    EXPECT_NEAR(
        world_low.resource_at(x, y),
        initial_resource - 0.5f,
        1e-5f);

    EXPECT_NEAR(
        world_high.resource_at(x, y),
        initial_resource - 2.0f,
        1e-5f);

    EXPECT_TRUE(low.is_alive());
    EXPECT_TRUE(high.is_alive());
}
