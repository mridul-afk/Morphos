
#include <gtest/gtest.h>

#include "morphos/simulation.hpp"

#include <cstddef>
#include <stdexcept>

using morphos::Simulation;
using morphos::SimulationConfig;

namespace
{
  SimulationConfig reproduction_config()
  {
    SimulationConfig config;

    config.enable_reproduction = true;
    config.initial_cell_energy = 20.0f;
    config.reproduction_threshold = 20.0f;

    config.absorption_rate = 0.0f;
    config.metabolism_rate = 0.0f;
    config.movement_speed = 0.0f;
    config.movement_cost = 0.0f;

    config.initial_resource_capacity = 0.0f;
    config.resource_regeneration_rate = 0.0f;

    config.mutation_probability = 0.0f;
    config.mutation_strength = 0.0f;

    config.max_population = 100;
    config.seed = 42;

    return config;
  }
}

TEST(PopulationTest, ReproductionIncreasesPopulation)
{
  Simulation sim(reproduction_config());
  sim.initialize(1);

  ASSERT_EQ(sim.living_population(), 1u);

  sim.step(1.0f);

  EXPECT_EQ(sim.living_population(), 2u);
  EXPECT_EQ(sim.population_size(), 2u);
  EXPECT_EQ(sim.total_births(), 1u);
  EXPECT_EQ(sim.total_deaths(), 0u);
}

TEST(PopulationTest, NewbornsAreNotUpdatedImmediately)
{
  Simulation sim(reproduction_config());
  sim.initialize(1);

  sim.step(1.0f);

  ASSERT_EQ(sim.cells().size(), 2u);

  EXPECT_EQ(sim.cells()[0].age(), 1u);
  EXPECT_EQ(sim.cells()[1].age(), 0u);
}

TEST(PopulationTest, ReproductionConservesEnergy)
{
  Simulation sim(reproduction_config());
  sim.initialize(1);

  const float initial_energy =
      sim.cells()[0].energy();

  sim.step(1.0f);

  ASSERT_EQ(sim.cells().size(), 2u);

  const float total_energy =
      sim.cells()[0].energy() +
      sim.cells()[1].energy();

  EXPECT_NEAR(total_energy, initial_energy, 1e-5f);
}

TEST(PopulationTest, PopulationLimitIsRespected)
{
  auto config = reproduction_config();

  config.initial_cell_energy = 40.0f;
  config.max_population = 5;

  Simulation sim(config);
  sim.initialize(4);

  for (int i = 0; i < 20; ++i)
  {
    sim.step(1.0f);
    EXPECT_LE(sim.living_population(), 5u);
  }

  EXPECT_EQ(sim.living_population(), 5u);
  EXPECT_EQ(sim.total_births(), 1u);
}

TEST(PopulationTest, TracksDeaths)
{
  auto config = reproduction_config();

  config.initial_cell_energy = 1.0f;
  config.metabolism_rate = 2.0f;

  Simulation sim(config);
  sim.initialize(3);

  sim.step(1.0f);

  EXPECT_EQ(sim.living_population(), 0u);
  EXPECT_EQ(sim.population_size(), 0u);
  EXPECT_EQ(sim.total_deaths(), 3u);
  EXPECT_EQ(sim.total_births(), 0u);
}

TEST(PopulationTest, ReproductionDisabledByDefault)
{
  SimulationConfig config;

  EXPECT_FALSE(config.enable_reproduction);

  Simulation sim(config);
  sim.initialize(10);

  for (int i = 0; i < 20; ++i)
  {
    sim.step(1.0f);
  }

  EXPECT_EQ(sim.population_size(), 10u);
  EXPECT_EQ(sim.total_births(), 0u);
}

TEST(PopulationTest, SameSeedProducesSameOffspring)
{
  auto config = reproduction_config();

  config.mutation_probability = 1.0f;
  config.mutation_strength = 0.2f;

  Simulation a(config);
  Simulation b(config);

  a.initialize(3);
  b.initialize(3);

  a.step(1.0f);
  b.step(1.0f);

  ASSERT_EQ(a.cells().size(), b.cells().size());

  for (std::size_t i = 0; i < a.cells().size(); ++i)
  {
    EXPECT_FLOAT_EQ(
        a.cells()[i].energy(),
        b.cells()[i].energy());

    EXPECT_FLOAT_EQ(
        a.cells()[i].genome().movement_speed,
        b.cells()[i].genome().movement_speed);

    EXPECT_FLOAT_EQ(
        a.cells()[i].genome().absorption_rate,
        b.cells()[i].genome().absorption_rate);
  }
}

TEST(PopulationTest, RejectsOversizedInitialPopulation)
{
  auto config = reproduction_config();
  config.max_population = 2;

  Simulation sim(config);

  EXPECT_THROW(
      sim.initialize(3),
      std::invalid_argument);
}

TEST(PopulationTest, ReinitializeResetsStatistics)
{
  Simulation sim(reproduction_config());

  sim.initialize(1);
  sim.step(1.0f);

  ASSERT_EQ(sim.total_births(), 1u);

  sim.initialize(1);

  EXPECT_EQ(sim.timestep(), 0u);
  EXPECT_EQ(sim.total_births(), 0u);
  EXPECT_EQ(sim.total_deaths(), 0u);
  EXPECT_EQ(sim.living_population(), 1u);
}
