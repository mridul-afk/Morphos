#include <gtest/gtest.h>
#include <cmath>

#include "morphos/cell.hpp"
#include "morphos/world.hpp"

using morphos::Cell;
using morphos::World;

TEST(MovementTest, StationaryCellDoesNotMove)
{
  World world(100, 100, 10, 10, 42);
  world.initialize_resources(50.0f);

  Cell cell(15, 15, 10, 0, 0, 0.0f, 0.05f);

  cell.update(world, 1.0f);

  EXPECT_FLOAT_EQ(cell.x(), 15.0f);
  EXPECT_FLOAT_EQ(cell.y(), 15.0f);
  EXPECT_FLOAT_EQ(cell.vx(), 0.0f);
  EXPECT_FLOAT_EQ(cell.vy(), 0.0f);
}

TEST(MovementTest, MovesTowardHigherResources)
{
  World world(100, 100, 10, 10, 42);
  world.initialize_resources(50.0f);

  // Eliminate resources in three neighboring cells.
  world.consume_resource(5, 15, 1000);
  world.consume_resource(15, 5, 1000);
  world.consume_resource(15, 25, 1000);

  // The right neighbor retains its initial resources.
  ASSERT_GT(world.resource_at(25, 15), 0.0f);

  Cell cell(15, 15, 10, 0, 0, 1.0f, 0.0f);

  cell.update(world, 1.0f);

  EXPECT_GT(cell.x(), 15.0f);
  EXPECT_NEAR(cell.y(), 15.0f, 1e-5f);
}

TEST(MovementTest, MovementConsumesEnergy)
{
  World world(100, 100, 10, 10, 42);
  world.initialize_resources(50.0f);

  world.consume_resource(5, 15, 1000);
  world.consume_resource(15, 5, 1000);
  world.consume_resource(15, 25, 1000);

  Cell cell(15, 15, 10, 0, 0, 1.0f, 0.5f);

  cell.update(world, 1.0f);

  EXPECT_NEAR(cell.energy(), 9.5f, 1e-4f);
}

TEST(MovementTest, WrapsAtWorldBoundary)
{
  World world(100, 100, 10, 10, 42);
  world.initialize_resources(50.0f);

  // At x=95, the right neighbor wraps to x=5.
  world.consume_resource(85, 15, 1000);
  world.consume_resource(95, 5, 1000);
  world.consume_resource(95, 25, 1000);

  ASSERT_GT(world.resource_at(5, 15), 0.0f);

  Cell cell(95, 15, 10, 0, 0, 10.0f, 0.0f);

  cell.update(world, 1.0f);

  EXPECT_NEAR(cell.x(), 5.0f, 1e-4f);
}

TEST(MovementTest, NoGradientMeansNoMovement)
{
  // A single resource cell is its own neighbor
  // in every direction under periodic boundaries.
  World world(100, 100, 1, 1, 42);
  world.initialize_resources(50.0f);

  Cell cell(50, 50, 10, 0, 0, 2.0f, 0.5f);

  cell.update(world, 1.0f);

  EXPECT_FLOAT_EQ(cell.x(), 50.0f);
  EXPECT_FLOAT_EQ(cell.y(), 50.0f);
  EXPECT_FLOAT_EQ(cell.energy(), 10.0f);
}

TEST(MovementTest, CannotSpendMoreEnergyThanAvailable)
{
  World world(100, 100, 10, 10, 42);
  world.initialize_resources(50.0f);

  // Create a positive x-direction gradient.
  world.consume_resource(5, 15, 1000);
  world.consume_resource(15, 5, 1000);
  world.consume_resource(15, 25, 1000);

  Cell cell(
      15, 15,
      1.0f,  // initial energy
      0.0f,  // absorption
      0.0f,  // metabolism
      10.0f, // desired speed
      1.0f   // movement cost
  );

  cell.update(world, 1.0f);

  // Maximum affordable speed = sqrt(1 / 1) = 1.
  EXPECT_NEAR(cell.x(), 16.0f, 1e-4f);
  EXPECT_NEAR(cell.energy(), 0.0f, 1e-4f);
}

TEST(MovementTest, ReservesEnergyForMetabolism)
{
  World world(100, 100, 10, 10, 42);
  world.initialize_resources(50.0f);

  world.consume_resource(5, 15, 1000);
  world.consume_resource(15, 5, 1000);
  world.consume_resource(15, 25, 1000);

  Cell cell(
      15, 15,
      5.0f,
      0.0f,
      3.0f,
      10.0f,
      1.0f);

  cell.update(world, 1.0f);

  // Available movement energy = 5 - 3 = 2.
  // Maximum affordable speed = sqrt(2).
  EXPECT_NEAR(
      cell.x(),
      15.0f + std::sqrt(2.0f),
      1e-4f);

  EXPECT_NEAR(cell.energy(), 0.0f, 1e-4f);
}

TEST(MovementTest, ZeroAvailableEnergyPreventsMovement)
{
  World world(100, 100, 10, 10, 42);
  world.initialize_resources(50.0f);

  Cell cell(
      15, 15,
      1.0f,
      0.0f,
      2.0f,
      10.0f,
      1.0f);

  cell.update(world, 1.0f);

  EXPECT_FLOAT_EQ(cell.x(), 15.0f);
  EXPECT_FLOAT_EQ(cell.y(), 15.0f);
  EXPECT_FALSE(cell.is_alive());
}

TEST(MovementTest, ZeroMovementCostAllowsFreeMovement)
{
  World world(100, 100, 10, 10, 42);
  world.initialize_resources(50.0f);

  world.consume_resource(5, 15, 1000);
  world.consume_resource(15, 5, 1000);
  world.consume_resource(15, 25, 1000);

  Cell cell(
      15, 15,
      1.0f,
      0.0f,
      0.0f,
      5.0f,
      0.0f);

  cell.update(world, 1.0f);

  EXPECT_NEAR(cell.x(), 20.0f, 1e-4f);
  EXPECT_NEAR(cell.energy(), 1.0f, 1e-4f);
}

TEST(MovementTest, TracksCumulativeDistance)
{
  World world(100, 100, 10, 10, 42);
  world.initialize_resources(50.0f);

  world.consume_resource(5, 15, 1000);
  world.consume_resource(15, 5, 1000);
  world.consume_resource(15, 25, 1000);

  Cell cell(15, 15, 10, 0, 0, 2.0f, 0.0f);

  EXPECT_FLOAT_EQ(cell.distance_traveled(), 0.0f);

  cell.update(world, 1.0f);

  EXPECT_NEAR(cell.distance_traveled(), 2.0f, 1e-4f);
}

TEST(MovementTest, TracksDistanceAcrossPeriodicBoundary)
{
  World world(100, 100, 10, 10, 42);
  world.initialize_resources(50.0f);

  world.consume_resource(85, 15, 1000);
  world.consume_resource(95, 5, 1000);
  world.consume_resource(95, 25, 1000);

  Cell cell(95, 15, 10, 0, 0, 10.0f, 0.0f);

  cell.update(world, 1.0f);

  EXPECT_NEAR(cell.x(), 5.0f, 1e-4f);
  EXPECT_NEAR(cell.distance_traveled(), 10.0f, 1e-4f);
}
