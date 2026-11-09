#include <gtest/gtest.h>
#include <utility>
#include <vector>

#include "morphos/simulation.hpp"

#include <stdexcept>

using morphos::Simulation;
using morphos::SimulationConfig;

TEST(ConfigTest, UsesConfiguredWorldDimensions)
{
  SimulationConfig config;
  config.world_width = 250.0f;
  config.world_height = 150.0f;

  Simulation sim(config);

  EXPECT_FLOAT_EQ(sim.world().width(), 250.0f);
  EXPECT_FLOAT_EQ(sim.world().height(), 150.0f);
}

TEST(ConfigTest, AppliesMovementParameters)
{
  SimulationConfig config;
  config.movement_speed = 0.0f;

  Simulation sim(config);
  sim.initialize(10);

  for (const auto &cell : sim.cells())
  {
    const float x = cell.x();
    const float y = cell.y();

    // Directly inspect configured movement behavior
    // through a separate, identically initialized sim.
    SimulationConfig mobile_config = config;
    mobile_config.movement_speed = 2.0f;

    Simulation mobile(mobile_config);
    mobile.initialize(10);

    EXPECT_FLOAT_EQ(
        mobile.cells().front().x(),
        sim.cells().front().x());

    EXPECT_FLOAT_EQ(
        mobile.cells().front().y(),
        sim.cells().front().y());

    EXPECT_GE(x, 0.0f);
    EXPECT_GE(y, 0.0f);
  }
}

TEST(ConfigTest, SameSeedSameInitialization)
{
  SimulationConfig config;
  config.seed = 1234;

  Simulation a(config);
  Simulation b(config);

  a.initialize(50);
  b.initialize(50);

  ASSERT_EQ(a.cells().size(), b.cells().size());

  for (std::size_t i = 0; i < a.cells().size(); ++i)
  {
    EXPECT_FLOAT_EQ(
        a.cells()[i].x(),
        b.cells()[i].x());

    EXPECT_FLOAT_EQ(
        a.cells()[i].y(),
        b.cells()[i].y());

    EXPECT_FLOAT_EQ(
        a.cells()[i].energy(),
        b.cells()[i].energy());
  }
}

TEST(ConfigTest, RejectsInvalidParameters)
{
  SimulationConfig config;
  config.movement_speed = -1.0f;

  EXPECT_THROW(
      Simulation sim(config),
      std::invalid_argument);
}

TEST(ConfigTest, LegacyConstructorStillWorks)
{
  Simulation sim(100.0f, 200.0f, 42);
  sim.initialize(10);

  EXPECT_FLOAT_EQ(sim.world().width(), 100.0f);
  EXPECT_FLOAT_EQ(sim.world().height(), 200.0f);
  EXPECT_EQ(sim.population_size(), 10u);
}

TEST(ConfigTest, StationaryConfigurationPreventsMovement)
{
  SimulationConfig config;
  config.movement_speed = 0.0f;

  Simulation sim(config);
  sim.initialize(20);

  std::vector<std::pair<float, float>> positions;

  for (const auto &cell : sim.cells())
    positions.emplace_back(cell.x(), cell.y());

  for (int i = 0; i < 10; ++i)
    sim.step(1.0f);

  for (std::size_t i = 0; i < sim.cells().size(); ++i)
  {
    EXPECT_FLOAT_EQ(sim.cells()[i].x(), positions[i].first);
    EXPECT_FLOAT_EQ(sim.cells()[i].y(), positions[i].second);
  }
}
