
#include <gtest/gtest.h>

#include "morphos/genome.hpp"

#include <random>
#include <stdexcept>

using morphos::Genome;

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
