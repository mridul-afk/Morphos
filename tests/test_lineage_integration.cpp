
#include <gtest/gtest.h>

#include "morphos/simulation.hpp"

#include <cstdint>

using morphos::Simulation;
using morphos::SimulationConfig;

namespace
{
  SimulationConfig lineage_config()
  {
    SimulationConfig config;

    config.enable_reproduction = true;
    config.initial_cell_energy = 40.0f;
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

    return config;
  }
}

TEST(LineageIntegrationTest, FoundersReceiveUniqueIds)
{
  Simulation sim(lineage_config());
  sim.initialize(3);

  ASSERT_EQ(sim.lineage_records().size(), 3u);

  for (std::uint64_t i = 0; i < 3; ++i)
  {
    const auto &cell = sim.cells()[i];

    EXPECT_EQ(cell.id(), i + 1);
    EXPECT_EQ(cell.parent_id(), 0u);
    EXPECT_EQ(cell.generation(), 0u);

    const auto *record = sim.find_lineage(i + 1);
    ASSERT_NE(record, nullptr);

    EXPECT_EQ(record->birth_timestep, 0u);
    EXPECT_TRUE(record->alive);
  }
}

TEST(LineageIntegrationTest, OffspringInheritsAncestry)
{
  Simulation sim(lineage_config());
  sim.initialize(1);

  sim.step(1.0f);

  ASSERT_EQ(sim.cells().size(), 2u);

  const auto &parent = sim.cells()[0];
  const auto &child = sim.cells()[1];

  EXPECT_EQ(parent.id(), 1u);
  EXPECT_EQ(child.id(), 2u);
  EXPECT_EQ(child.parent_id(), parent.id());
  EXPECT_EQ(child.generation(), 1u);

  const auto *record = sim.find_lineage(child.id());
  ASSERT_NE(record, nullptr);

  EXPECT_EQ(record->parent_id, parent.id());
  EXPECT_EQ(record->birth_timestep, 1u);
}

TEST(LineageIntegrationTest, TracksMultipleGenerations)
{
  Simulation sim(lineage_config());
  sim.initialize(1);

  sim.step(1.0f);
  sim.step(1.0f);

  // Founder divides at step 1.
  // Both cells have enough energy to divide at step 2.
  ASSERT_EQ(sim.cells().size(), 4u);

  EXPECT_EQ(sim.cells()[0].generation(), 0u);
  EXPECT_EQ(sim.cells()[1].generation(), 1u);
  EXPECT_EQ(sim.cells()[2].generation(), 1u);
  EXPECT_EQ(sim.cells()[3].generation(), 2u);

  EXPECT_EQ(sim.cells()[3].parent_id(), 2u);
}

TEST(LineageIntegrationTest, DeathPreservesHistory)
{
  auto config = lineage_config();

  config.initial_cell_energy = 1.0f;
  config.metabolism_rate = 2.0f;

  Simulation sim(config);
  sim.initialize(2);

  sim.step(1.0f);

  EXPECT_EQ(sim.population_size(), 0u);
  EXPECT_EQ(sim.lineage_records().size(), 2u);

  for (std::uint64_t id = 1; id <= 2; ++id)
  {
    const auto *record = sim.find_lineage(id);
    ASSERT_NE(record, nullptr);

    EXPECT_FALSE(record->alive);
    EXPECT_EQ(record->death_timestep, 1u);
  }
}

TEST(LineageIntegrationTest, ReinitializeResetsLineage)
{
  Simulation sim(lineage_config());

  sim.initialize(1);
  sim.step(1.0f);

  ASSERT_EQ(sim.lineage_records().size(), 2u);

  sim.initialize(1);

  ASSERT_EQ(sim.lineage_records().size(), 1u);

  EXPECT_EQ(sim.cells()[0].id(), 1u);
  EXPECT_EQ(sim.cells()[0].parent_id(), 0u);
  EXPECT_EQ(sim.cells()[0].generation(), 0u);

  EXPECT_EQ(sim.find_lineage(2), nullptr);
}

TEST(LineageIntegrationTest, UnknownIdReturnsNull)
{
  Simulation sim(lineage_config());
  sim.initialize(1);

  EXPECT_EQ(sim.find_lineage(0), nullptr);
  EXPECT_EQ(sim.find_lineage(999), nullptr);
}
