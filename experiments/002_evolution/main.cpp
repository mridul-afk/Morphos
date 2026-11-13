#include "morphos/simulation.hpp"
#include "morphos/lineage_analytics.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace
{
  constexpr std::size_t INITIAL_POPULATION = 100;
  constexpr std::size_t MAX_POPULATION = 1000;
  constexpr std::uint64_t STEPS = 500;
  constexpr std::uint32_t TRIALS = 10;
  constexpr float DT = 1.0f;
  constexpr std::uint32_t BASE_SEED = 42;

  morphos::SimulationConfig make_config(
      std::uint32_t seed)
  {
    morphos::SimulationConfig config;

    config.seed = seed;

    config.enable_reproduction = true;
    config.max_population = MAX_POPULATION;

    config.initial_cell_energy = 40.0f;
    config.reproduction_threshold = 20.0f;

    config.movement_speed = 1.0f;
    config.absorption_rate = 2.0f;

    config.mutation_probability = 0.0f;
    config.mutation_strength = 0.0f;

    // Other environmental and metabolic parameters
    // retain their SimulationConfig defaults.

    return config;
  }

  void verify_stream(const std::ofstream &out)
  {
    if (!out)
    {
      throw std::runtime_error(
          "Failed writing experiment output");
    }
  }

  void run_trial(
      std::uint32_t trial,
      const fs::path &output_directory,
      std::ofstream &summary)
  {
    const std::uint32_t seed = BASE_SEED + trial;

    const fs::path trial_dir =
        output_directory /
        ("trial_" + std::to_string(trial));

    fs::create_directories(trial_dir);

    const auto config = make_config(seed);

    morphos::Simulation simulation(config);

    simulation.initialize(INITIAL_POPULATION);

    std::ofstream trajectory(
        trial_dir / "population.csv");

    if (!trajectory)
    {
      throw std::runtime_error(
          "Unable to open population.csv");
    }

    trajectory
        << "timestep,population,living,"
           "births,deaths\n";

    auto record_population = [&]()
    {
      trajectory
          << simulation.timestep() << ','
          << simulation.population_size() << ','
          << simulation.living_population() << ','
          << simulation.total_births() << ','
          << simulation.total_deaths()
          << '\n';

      verify_stream(trajectory);
    };

    // Record initial population.
    record_population();

    for (std::uint64_t step = 0;
         step < STEPS;
         ++step)
    {
      simulation.step(DT);
      record_population();
    }

    trajectory.close();

    const auto &records =
        simulation.lineage_records();

    const auto stats =
        morphos::LineageAnalytics::analyze(records);

    morphos::LineageAnalytics::export_csv(
        records,
        trial_dir / "lineage.csv");

    morphos::LineageAnalytics::export_summary_csv(
        stats,
        trial_dir / "lineage_summary.csv");

    // Count surviving founding lineages.
    std::size_t surviving_lineages = 0;

    for (const auto &stat : stats)
    {
      if (stat.lineage_survives)
      {
        ++surviving_lineages;
      }
    }

    const auto max_generation =
        morphos::LineageAnalytics::
            maximum_generation(records);

    summary
        << trial << ','
        << seed << ','
        << simulation.living_population() << ','
        << simulation.total_births() << ','
        << simulation.total_deaths() << ','
        << records.size() << ','
        << surviving_lineages << ','
        << max_generation
        << '\n';

    verify_stream(summary);

    // Save the effective experimental settings.
    std::ofstream metadata(
        trial_dir / "config.csv");

    if (!metadata)
    {
      throw std::runtime_error(
          "Unable to open config.csv");
    }

    metadata << "parameter,value\n";

    metadata << "seed," << config.seed << '\n';
    metadata << "initial_population,"
             << INITIAL_POPULATION << '\n';
    metadata << "max_population,"
             << config.max_population << '\n';
    metadata << "steps," << STEPS << '\n';
    metadata << "dt," << DT << '\n';

    metadata << "world_width,"
             << config.world_width << '\n';
    metadata << "world_height,"
             << config.world_height << '\n';
    metadata << "grid_width,"
             << config.grid_width << '\n';
    metadata << "grid_height,"
             << config.grid_height << '\n';

    metadata << "initial_resource_capacity,"
             << config.initial_resource_capacity << '\n';
    metadata << "resource_regeneration_rate,"
             << config.resource_regeneration_rate << '\n';
    metadata << "resource_smoothing_passes,"
             << config.resource_smoothing_passes << '\n';

    metadata << "initial_cell_energy,"
             << config.initial_cell_energy << '\n';
    metadata << "absorption_rate,"
             << config.absorption_rate << '\n';
    metadata << "metabolism_rate,"
             << config.metabolism_rate << '\n';
    metadata << "movement_speed,"
             << config.movement_speed << '\n';
    metadata << "movement_cost,"
             << config.movement_cost << '\n';

    metadata << "enable_reproduction,"
             << (config.enable_reproduction ? 1 : 0)
             << '\n';
    metadata << "reproduction_threshold,"
             << config.reproduction_threshold << '\n';
    metadata << "mutation_probability,"
             << config.mutation_probability << '\n';
    metadata << "mutation_strength,"
             << config.mutation_strength << '\n';

    metadata.flush();
    verify_stream(metadata);

    std::cout
        << "Trial " << trial
        << " | Seed " << seed
        << " | Living " << simulation.living_population()
        << " | Births " << simulation.total_births()
        << " | Deaths " << simulation.total_deaths()
        << " | Surviving lineages "
        << surviving_lineages
        << " | Max generation "
        << max_generation
        << '\n';
  }
}

int main(int argc, char **argv)
{
  try
  {
    const fs::path output_directory =
        argc >= 2
            ? fs::path(argv[1])
            : fs::path("results_002A");

    fs::create_directories(output_directory);

    std::ofstream summary(
        output_directory / "summary.csv");

    if (!summary)
    {
      throw std::runtime_error(
          "Unable to create summary.csv");
    }

    summary
        << "trial,seed,final_living_population,"
           "total_births,total_deaths,"
           "total_organisms,"
           "surviving_founder_lineages,"
           "maximum_generation\n";

    for (std::uint32_t trial = 0;
         trial < TRIALS;
         ++trial)
    {
      run_trial(
          trial,
          output_directory,
          summary);
    }

    summary.flush();
    verify_stream(summary);

    std::cout
        << "\nExperiment 002A completed successfully.\n"
        << "Output: " << output_directory << '\n';

    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr
        << "Experiment 002A failed: "
        << error.what()
        << '\n';

    return 1;
  }
}
