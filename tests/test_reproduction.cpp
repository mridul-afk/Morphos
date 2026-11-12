#include <gtest/gtest.h>

#include "morphos/cell.hpp"
#include "morphos/genome.hpp"

#include <random>
#include <stdexcept>

using morphos::Cell;
using morphos::Genome;

// Successful reproduction must conserve cell energy.
TEST(ReproductionTest, ConservesEnergy)
{
  std::mt19937 rng(42);

  Cell parent(
      10.0f, 20.0f,
      20.0f, 2.0f, 1.0f, 1.0f, 0.05f);

  const float energy_before = parent.energy();

  auto offspring = parent.reproduce(
      rng, 15.0f, 0.0f, 0.0f);

  ASSERT_TRUE(offspring.has_value());

  EXPECT_FLOAT_EQ(parent.energy(), 10.0f);
  EXPECT_FLOAT_EQ(offspring->energy(), 10.0f);

  EXPECT_NEAR(
      parent.energy() + offspring->energy(),
      energy_before,
      1e-5f);
}

// Insufficient energy must prevent reproduction.
TEST(ReproductionTest, InsufficientEnergy)
{
  std::mt19937 rng(42);

  Cell parent(
      10.0f, 20.0f,
      10.0f, 2.0f, 1.0f, 1.0f, 0.05f);

  auto offspring = parent.reproduce(
      rng, 15.0f);

  EXPECT_FALSE(offspring.has_value());
  EXPECT_FLOAT_EQ(parent.energy(), 10.0f);
}

// Dead cells cannot reproduce.
TEST(ReproductionTest, DeadCellCannotReproduce)
{
  std::mt19937 rng(42);

  Cell parent(
      10.0f, 20.0f,
      0.0f, 2.0f, 1.0f, 1.0f, 0.05f);

  auto offspring = parent.reproduce(
      rng, 15.0f);

  EXPECT_FALSE(offspring.has_value());
}

// Without mutation, offspring must inherit both genes.
TEST(ReproductionTest, InheritsGenome)
{
  std::mt19937 rng(42);

  Cell parent(
      10.0f, 20.0f,
      20.0f, 2.0f, 1.0f, 0.25f, 0.05f);

  auto offspring = parent.reproduce(
      rng, 15.0f, 0.0f, 0.0f);

  ASSERT_TRUE(offspring.has_value());

  EXPECT_FLOAT_EQ(
      offspring->genome().movement_speed,
      parent.genome().movement_speed);

  EXPECT_FLOAT_EQ(
      offspring->genome().absorption_rate,
      parent.genome().absorption_rate);

  EXPECT_EQ(offspring->age(), 0u);
  EXPECT_TRUE(offspring->is_alive());
}

// Reproduction must not modify the parent's genome.
TEST(ReproductionTest, ParentGenomeRemainsUnchanged)
{
  std::mt19937 rng(42);

  Cell parent(
      10.0f, 20.0f,
      20.0f, 2.0f, 1.0f, 0.25f, 0.05f);

  const Genome original = parent.genome();

  auto offspring = parent.reproduce(
      rng, 15.0f, 1.0f, 0.5f);

  ASSERT_TRUE(offspring.has_value());

  EXPECT_FLOAT_EQ(
      parent.genome().movement_speed,
      original.movement_speed);

  EXPECT_FLOAT_EQ(
      parent.genome().absorption_rate,
      original.absorption_rate);

  EXPECT_NO_THROW(offspring->genome().validate());
}

// Reproduction must be reproducible with the same seed.
TEST(ReproductionTest, DeterministicWithSameSeed)
{
  std::mt19937 rng_a(42);
  std::mt19937 rng_b(42);

  Cell parent_a(
      10.0f, 20.0f,
      20.0f, 2.0f, 1.0f, 0.25f, 0.05f);

  Cell parent_b = parent_a;

  auto child_a = parent_a.reproduce(
      rng_a, 15.0f, 1.0f, 0.5f);

  auto child_b = parent_b.reproduce(
      rng_b, 15.0f, 1.0f, 0.5f);

  ASSERT_TRUE(child_a.has_value());
  ASSERT_TRUE(child_b.has_value());

  EXPECT_FLOAT_EQ(
      child_a->genome().movement_speed,
      child_b->genome().movement_speed);

  EXPECT_FLOAT_EQ(
      child_a->genome().absorption_rate,
      child_b->genome().absorption_rate);
}

// Invalid reproduction settings must be rejected.
TEST(ReproductionTest, RejectsInvalidThreshold)
{
  std::mt19937 rng(42);

  Cell parent(
      10.0f, 20.0f,
      20.0f, 2.0f, 1.0f, 1.0f, 0.05f);

  EXPECT_THROW(
      (void)parent.reproduce(rng, -1.0f),
      std::invalid_argument);

  EXPECT_THROW(
      (void)parent.reproduce(rng, 0.0f),
      std::invalid_argument);
}
