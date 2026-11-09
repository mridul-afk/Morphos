#include <gtest/gtest.h>

#include "morphos/world.hpp"

#include <stdexcept>

using morphos::World;

TEST(WorldTest, Dimensions)
{
  World world(100.0f, 200.0f);

  EXPECT_FLOAT_EQ(world.width(), 100.0f);
  EXPECT_FLOAT_EQ(world.height(), 200.0f);
}

TEST(WorldTest, RejectsInvalidDimensions)
{
  EXPECT_THROW(World(0.0f, 100.0f),
               std::invalid_argument);

  EXPECT_THROW(World(100.0f, -1.0f),
               std::invalid_argument);
}

TEST(WorldTest, ResourcesWithinCapacity)
{
  World world(100.0f, 100.0f, 10, 10);
  world.initialize_resources(50.0f);

  for (int x = 5; x < 100; x += 10)
  {
    for (int y = 5; y < 100; y += 10)
    {
      const float energy = world.resource_at(
          static_cast<float>(x),
          static_cast<float>(y));

      EXPECT_GE(energy, 0.0f);
      EXPECT_LE(energy, 50.0f);
    }
  }
}

TEST(WorldTest, ConsumptionConservesResources)
{
  World world(100.0f, 100.0f, 10, 10);
  world.initialize_resources(50.0f);

  const float before = world.resource_at(5, 5);
  const float consumed =
      world.consume_resource(5, 5, 10.0f);
  const float after = world.resource_at(5, 5);

  EXPECT_NEAR(before - after, consumed, 1e-4f);
  EXPECT_LE(consumed, 10.0f);
  EXPECT_GE(after, 0.0f);
}

TEST(WorldTest, CannotOverconsume)
{
  World world(100.0f, 100.0f, 10, 10);
  world.initialize_resources(50.0f);

  const float available = world.resource_at(5, 5);
  const float consumed =
      world.consume_resource(5, 5, 1000.0f);

  EXPECT_FLOAT_EQ(consumed, available);
  EXPECT_FLOAT_EQ(world.resource_at(5, 5), 0.0f);
}

TEST(WorldTest, RegenerationRespectsCapacity)
{
  World world(100.0f, 100.0f, 10, 10);
  world.initialize_resources(50.0f);

  world.consume_resource(5, 5, 1000.0f);
  world.regenerate_resources(100.0f, 1.0f);

  EXPECT_FLOAT_EQ(world.resource_at(5, 5), 50.0f);
}

TEST(WorldTest, PeriodicBoundaries)
{
  World world(100.0f, 100.0f, 10, 10);
  world.initialize_resources(50.0f);

  EXPECT_FLOAT_EQ(
      world.resource_at(5, 5),
      world.resource_at(105, 5));

  EXPECT_FLOAT_EQ(
      world.resource_at(5, 5),
      world.resource_at(-95, 5));
}

TEST(WorldTest, SameSeedProducesSameResources)
{
  World a(100, 100, 10, 10, 42);
  World b(100, 100, 10, 10, 42);

  a.initialize_resources(50.0f);
  b.initialize_resources(50.0f);

  for (int x = 5; x < 100; x += 10)
  {
    for (int y = 5; y < 100; y += 10)
    {
      EXPECT_FLOAT_EQ(
          a.resource_at(x, y),
          b.resource_at(x, y));
    }
  }
}

TEST(WorldTest, SmoothingPreservesApproximateTotalResources)
{
  World world(100, 100, 16, 16, 42);
  world.initialize_resources(100.0f);

  auto total_resources = [&world]()
  {
    double total = 0.0;

    for (int y = 0; y < 16; ++y)
    {
      for (int x = 0; x < 16; ++x)
      {
        total += world.resource_at(
            (x + 0.5f) * world.cell_width(),
            (y + 0.5f) * world.cell_height());
      }
    }

    return total;
  };

  const double before = total_resources();

  world.smooth_resources(16);

  const double after = total_resources();

  EXPECT_NEAR(before, after, before * 1e-4);
}
