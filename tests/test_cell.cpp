#include <gtest/gtest.h>

#include "morphos/cell.hpp"
#include "morphos/world.hpp"

using morphos::Cell;
using morphos::World;

TEST(CellTest, InitializesCorrectly)
{
  Cell cell(10.0f, 20.0f, 15.0f, 2.0f, 1.0f);

  EXPECT_FLOAT_EQ(cell.x(), 10.0f);
  EXPECT_FLOAT_EQ(cell.y(), 20.0f);
  EXPECT_FLOAT_EQ(cell.energy(), 15.0f);
  EXPECT_TRUE(cell.is_alive());
  EXPECT_EQ(cell.age(), 0u);
}

TEST(CellTest, MetabolismConsumesEnergy)
{
  World world(100, 100, 10, 10);
  world.initialize_resources(0.0f);

  Cell cell(5, 5, 10.0f, 0.0f, 1.0f);

  cell.update(world, 1.0f);

  EXPECT_FLOAT_EQ(cell.energy(), 9.0f);
  EXPECT_EQ(cell.age(), 1u);
}

TEST(CellTest, AbsorbsEnvironmentalEnergy)
{
  World world(100, 100, 10, 10);
  world.initialize_resources(50.0f);

  Cell cell(5, 5, 10.0f, 2.0f, 0.0f);

  const float before = world.resource_at(5, 5);
  const float initial_energy = cell.energy();

  cell.update(world, 1.0f);

  const float absorbed = before - world.resource_at(5, 5);

  EXPECT_NEAR(
      cell.energy(),
      initial_energy + absorbed,
      1e-4f);
}

TEST(CellTest, DiesFromStarvation)
{
  World world(100, 100, 10, 10);
  world.initialize_resources(0.0f);

  Cell cell(5, 5, 2.0f, 0.0f, 1.0f);

  cell.update(world, 1.0f);
  EXPECT_TRUE(cell.is_alive());

  cell.update(world, 1.0f);

  EXPECT_FALSE(cell.is_alive());
  EXPECT_FLOAT_EQ(cell.energy(), 0.0f);
}

TEST(CellTest, DeadCellsRemainInactive)
{
  World world(100, 100, 10, 10);
  world.initialize_resources(50.0f);

  Cell cell(5, 5, 0.0f, 2.0f, 1.0f);

  const float before = world.resource_at(5, 5);

  cell.update(world, 1.0f);

  EXPECT_FALSE(cell.is_alive());
  EXPECT_FLOAT_EQ(world.resource_at(5, 5), before);
  EXPECT_EQ(cell.age(), 0u);
}
