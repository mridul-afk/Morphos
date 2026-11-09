
#include "morphos/simulation.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using morphos::Simulation;
using morphos::SimulationConfig;

namespace
{

  constexpr std::size_t kPopulation = 100;
  constexpr int kSteps = 500;
  constexpr int kTrials = 30;
  constexpr float kDt = 1.0f;
  constexpr std::uint32_t kBaseSeed = 42;

  struct Metrics
  {
    std::size_t living = 0;
    double survival_percent = 0.0;
    double mean_living_energy = 0.0;
    double total_population_energy = 0.0;
    double total_distance = 0.0;
  };

  Metrics measure(const Simulation &sim)
  {
    Metrics m;

    double living_energy = 0.0;

    for (const auto &cell : sim.cells())
    {
      m.total_population_energy += cell.energy();
      m.total_distance += cell.distance_traveled();

      if (cell.is_alive())
      {
        ++m.living;
        living_energy += cell.energy();
      }
    }

    const std::size_t population = sim.population_size();

    if (population > 0)
    {
      m.survival_percent =
          100.0 * static_cast<double>(m.living) /
          static_cast<double>(population);
    }

    if (m.living > 0)
    {
      m.mean_living_energy =
          living_energy / static_cast<double>(m.living);
    }

    return m;
  }

  void write_trajectory(
      std::ofstream &file,
      int trial,
      std::uint32_t seed,
      const std::string &group,
      int step,
      const Metrics &m)
  {
    file << trial << ','
         << seed << ','
         << group << ','
         << step << ','
         << m.living << ','
         << m.survival_percent << ','
         << m.mean_living_energy << ','
         << m.total_population_energy << ','
         << m.total_distance << '\n';
  }

  void write_summary(
      std::ofstream &file,
      int trial,
      std::uint32_t seed,
      const std::string &group,
      const Metrics &m)
  {
    file << trial << ','
         << seed << ','
         << group << ','
         << m.living << ','
         << m.survival_percent << ','
         << m.mean_living_energy << ','
         << m.total_population_energy << ','
         << m.total_distance << '\n';
  }

  void verify_paired_initialization(
      const Simulation &stationary,
      const Simulation &mobile)
  {
    const auto &a = stationary.cells();
    const auto &b = mobile.cells();

    if (a.size() != b.size())
    {
      throw std::runtime_error(
          "Paired populations have different sizes");
    }

    for (std::size_t i = 0; i < a.size(); ++i)
    {
      if (a[i].x() != b[i].x() ||
          a[i].y() != b[i].y() ||
          a[i].energy() != b[i].energy())
      {
        throw std::runtime_error(
            "Paired initial cell states differ");
      }
    }

    // Check the resource field at every grid-cell center.
    const auto &wa = stationary.world();
    const auto &wb = mobile.world();

    const auto &config = stationary.config();

    const float dx =
        config.world_width /
        static_cast<float>(config.grid_width);

    const float dy =
        config.world_height /
        static_cast<float>(config.grid_height);

    for (std::size_t y = 0; y < config.grid_height; ++y)
    {
      for (std::size_t x = 0; x < config.grid_width; ++x)
      {
        const float px =
            (static_cast<float>(x) + 0.5f) * dx;

        const float py =
            (static_cast<float>(y) + 0.5f) * dy;

        if (wa.resource_at(px, py) !=
            wb.resource_at(px, py))
        {
          throw std::runtime_error(
              "Paired initial resource fields differ");
        }
      }
    }
  }

} // anonymous namespace

int main(int argc, char **argv)
{
  try
  {
    // Optional output directory.
    fs::path output_dir =
        argc > 1 ? fs::path(argv[1])
                 : fs::path("experiments/001_selection/results");

    fs::create_directories(output_dir);

    std::ofstream trajectories(
        output_dir / "trajectories.csv");

    std::ofstream summary(
        output_dir / "summary.csv");

    if (!trajectories || !summary)
    {
      throw std::runtime_error(
          "Unable to open experiment CSV files");
    }

    trajectories << std::setprecision(10);
    summary << std::setprecision(10);

    trajectories
        << "trial,seed,group,step,living,"
        << "survival_percent,mean_living_energy,"
        << "total_population_energy,total_distance\n";

    summary
        << "trial,seed,group,living,"
        << "survival_percent,mean_living_energy,"
        << "total_population_energy,total_distance\n";

    SimulationConfig base;
    base.world_width = 100.0f;
    base.world_height = 100.0f;

    base.grid_width = 128;
    base.grid_height = 128;

    base.initial_resource_capacity = 100.0f;
    base.resource_regeneration_rate = 0.01f;

    base.initial_cell_energy = 10.0f;
    base.absorption_rate = 2.0f;
    base.metabolism_rate = 1.0f;

    base.movement_cost = 0.05f;

    std::cout << "MORPHOS Experiment 001\n";
    std::cout << "Population: " << kPopulation << '\n';
    std::cout << "Steps: " << kSteps << '\n';
    std::cout << "Trials: " << kTrials << "\n\n";

    double stationary_survival_sum = 0.0;
    double mobile_survival_sum = 0.0;

    for (int trial = 0; trial < kTrials; ++trial)
    {
      const std::uint32_t seed =
          kBaseSeed + static_cast<std::uint32_t>(trial);

      SimulationConfig stationary_config = base;
      stationary_config.seed = seed;
      stationary_config.movement_speed = 0.0f;

      SimulationConfig mobile_config = base;
      mobile_config.seed = seed;
      mobile_config.movement_speed = 1.0f;

      Simulation stationary(stationary_config);
      Simulation mobile(mobile_config);

      stationary.initialize(kPopulation);
      mobile.initialize(kPopulation);

      verify_paired_initialization(stationary, mobile);

      // Record initial conditions (step 0).
      write_trajectory(
          trajectories, trial + 1, seed,
          "stationary", 0, measure(stationary));

      write_trajectory(
          trajectories, trial + 1, seed,
          "mobile", 0, measure(mobile));

      for (int step = 1; step <= kSteps; ++step)
      {
        stationary.step(kDt);
        mobile.step(kDt);

        write_trajectory(
            trajectories, trial + 1, seed,
            "stationary", step,
            measure(stationary));

        write_trajectory(
            trajectories, trial + 1, seed,
            "mobile", step,
            measure(mobile));
      }

      const Metrics stationary_final =
          measure(stationary);

      const Metrics mobile_final =
          measure(mobile);

      write_summary(
          summary, trial + 1, seed,
          "stationary", stationary_final);

      write_summary(
          summary, trial + 1, seed,
          "mobile", mobile_final);

      stationary_survival_sum +=
          stationary_final.survival_percent;

      mobile_survival_sum +=
          mobile_final.survival_percent;

      std::cout
          << "Trial " << (trial + 1)
          << " | seed=" << seed
          << " | stationary=" << stationary_final.living
          << " | mobile=" << mobile_final.living
          << '\n';
    }

    const double stationary_mean =
        stationary_survival_sum / kTrials;

    const double mobile_mean =
        mobile_survival_sum / kTrials;

    std::cout << "\nExperiment completed.\n";

    std::cout << std::fixed << std::setprecision(2);

    std::cout << "Mean final survival (stationary): "
              << stationary_mean << "%\n";

    std::cout << "Mean final survival (mobile): "
              << mobile_mean << "%\n";

    std::cout << "Difference (mobile - stationary): "
              << mobile_mean - stationary_mean
              << " percentage points\n";

    std::cout << "\nResults: "
              << fs::absolute(output_dir).string()
              << '\n';

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Experiment failed: "
              << e.what() << '\n';

    return 1;
  }
}
