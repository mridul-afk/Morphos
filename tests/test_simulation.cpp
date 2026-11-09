#include <gtest/gtest.h>

#include "morphos/simulation.hpp"

using morphos::Simulation;

TEST(SimulationTest, InitializesPopulation)
{
  Simulation simulation(100, 100, 42);

  simulation.initialize(100);

  EXPECT_EQ(simulation.population_size(), 100u);
  EXPECT_EQ(simulation.living_population(), 100u);
  EXPECT_EQ(simulation.timestep(), 0u);
}

TEST(SimulationTest, AdvancesTime)
{
  Simulation simulation(100, 100, 42);
  simulation.initialize(10);

  simulation.step(1.0f);
  simulation.step(1.0f);

  EXPECT_EQ(simulation.timestep(), 2u);
}

TEST(SimulationTest, ZeroPopulation)
{
  Simulation simulation(100, 100, 42);
  simulation.initialize(0);

  EXPECT_EQ(simulation.population_size(), 0u);
  EXPECT_EQ(simulation.living_population(), 0u);
}

TEST(SimulationTest, SameSeedSameInitialization)
{
  Simulation a(100, 100, 42);
  Simulation b(100, 100, 42);

  a.initialize(20);
  b.initialize(20);

  ASSERT_EQ(a.cells().size(), b.cells().size());

  for (std::size_t i = 0; i < a.cells().size(); ++i)
  {
    EXPECT_FLOAT_EQ(a.cells()[i].x(), b.cells()[i].x());
    EXPECT_FLOAT_EQ(a.cells()[i].y(), b.cells()[i].y());
    EXPECT_FLOAT_EQ(a.cells()[i].energy(),
                    b.cells()[i].energy());
  }
}

TEST(SimulationTest, LivingPopulationNeverIncreasesYet)
{
  Simulation simulation(100, 100, 42);
  simulation.initialize(100);

  std::size_t previous = simulation.living_population();

  for (int i = 0; i < 100; ++i)
  {
    simulation.step(1.0f);

    EXPECT_LE(simulation.living_population(), previous);

    previous = simulation.living_population();
  }
}
