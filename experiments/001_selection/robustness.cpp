
#include "morphos/simulation.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

using morphos::Simulation;
using morphos::SimulationConfig;

namespace
{

  constexpr int TRIALS = 10;
  constexpr std::size_t POPULATION = 100;
  constexpr double DURATION = 500.0;
  constexpr std::uint32_t BASE_SEED = 42;

  constexpr std::array<float, 3> SPEEDS = {
      0.0f, 0.25f, 4.0f};

  struct Scenario
  {
    const char *name;
    std::size_t grid;
    std::size_t smoothing;
    float dt;
  };

  // One baseline and six controlled modifications.
  constexpr std::array<Scenario, 7> SCENARIOS = {{{"baseline", 128, 0, 1.0f},
                                                  {"grid_64", 64, 0, 1.0f},
                                                  {"grid_256", 256, 0, 1.0f},
                                                  {"smooth_4", 128, 4, 1.0f},
                                                  {"smooth_16", 128, 16, 1.0f},
                                                  {"dt_050", 128, 0, 0.5f},
                                                  {"dt_025", 128, 0, 0.25f}}};

  struct Metrics
  {
    std::size_t living = 0;
    double survival = 0.0;
    double total_energy = 0.0;
    double mean_living_energy = 0.0;
    double mean_distance = 0.0;
  };

  Metrics measure(const Simulation &sim)
  {
    Metrics m;
    double living_energy = 0.0;
    double distance = 0.0;

    for (const auto &cell : sim.cells())
    {
      m.total_energy += cell.energy();
      distance += cell.distance_traveled();

      if (cell.is_alive())
      {
        ++m.living;
        living_energy += cell.energy();
      }
    }

    const auto n = sim.population_size();

    if (n > 0)
    {
      m.survival = 100.0 * m.living / n;
      m.mean_distance = distance / n;
    }

    if (m.living > 0)
    {
      m.mean_living_energy = living_energy / m.living;
    }

    return m;
  }

  SimulationConfig make_config(
      const Scenario &scenario,
      float speed,
      std::uint32_t seed)
  {
    SimulationConfig c;

    c.world_width = 100.0f;
    c.world_height = 100.0f;

    c.grid_width = scenario.grid;
    c.grid_height = scenario.grid;

    c.initial_resource_capacity = 100.0f;
    c.resource_regeneration_rate = 0.01f;
    c.resource_smoothing_passes = scenario.smoothing;

    c.initial_cell_energy = 10.0f;
    c.absorption_rate = 2.0f;
    c.metabolism_rate = 1.0f;

    c.movement_speed = speed;
    c.movement_cost = 0.05f;

    c.seed = seed;

    return c;
  }

  void write_row(
      std::ofstream &file,
      const Scenario &scenario,
      int trial,
      std::uint32_t seed,
      float speed,
      int step,
      const Metrics &m)
  {
    file << scenario.name << ','
         << scenario.grid << ','
         << scenario.smoothing << ','
         << scenario.dt << ','
         << trial << ','
         << seed << ','
         << speed << ','
         << step << ','
         << step * static_cast<double>(scenario.dt) << ','
         << m.living << ','
         << m.survival << ','
         << m.total_energy << ','
         << m.mean_living_energy << ','
         << m.mean_distance << '\n';
  }

} // namespace

int main(int argc, char **argv)
{
  try
  {
    const fs::path output_dir =
        argc > 1
            ? fs::path(argv[1])
            : fs::path(
                  "experiments/001_selection/results_001C");

    fs::create_directories(output_dir);

    std::ofstream summary(
        output_dir / "robustness_summary.csv");

    std::ofstream trajectories(
        output_dir / "robustness_trajectories.csv");

    if (!summary || !trajectories)
    {
      throw std::runtime_error(
          "Could not open output CSV files");
    }

    summary << std::setprecision(10);
    trajectories << std::setprecision(10);

    const char *header =
        "scenario,grid,smoothing,dt,trial,seed,"
        "speed,step,time,living,survival_percent,"
        "total_energy,mean_living_energy,mean_distance\n";

    summary << header;
    trajectories << header;

    std::cout << "MORPHOS Experiment 001C\n";
    std::cout << "Environmental and numerical robustness\n";
    std::cout << "Scenarios: " << SCENARIOS.size() << '\n';
    std::cout << "Speeds: " << SPEEDS.size() << '\n';
    std::cout << "Trials per combination: " << TRIALS << '\n';
    std::cout << "Total runs: "
              << SCENARIOS.size() * SPEEDS.size() * TRIALS
              << "\n\n";

    for (const Scenario &scenario : SCENARIOS)
    {
      const int steps = static_cast<int>(
          std::lround(DURATION / scenario.dt));

      for (float speed : SPEEDS)
      {
        double survival_sum = 0.0;
        double energy_sum = 0.0;
        double distance_sum = 0.0;

        for (int trial = 0; trial < TRIALS; ++trial)
        {
          const std::uint32_t seed =
              BASE_SEED +
              static_cast<std::uint32_t>(trial);

          Simulation sim(
              make_config(scenario, speed, seed));

          sim.initialize(POPULATION);

          if (sim.living_population() != POPULATION)
          {
            throw std::runtime_error(
                "Unexpected initial population");
          }

          write_row(
              trajectories, scenario,
              trial + 1, seed, speed,
              0, measure(sim));

          for (int step = 1; step <= steps; ++step)
          {
            sim.step(scenario.dt);

            write_row(
                trajectories, scenario,
                trial + 1, seed, speed,
                step, measure(sim));
          }

          const Metrics final = measure(sim);

          write_row(
              summary, scenario,
              trial + 1, seed, speed,
              steps, final);

          survival_sum += final.survival;
          energy_sum += final.total_energy;
          distance_sum += final.mean_distance;
        }

        std::cout << std::fixed
                  << std::setprecision(2)
                  << scenario.name
                  << " | speed=" << speed
                  << " | survival="
                  << survival_sum / TRIALS << "%"
                  << " | total_energy="
                  << energy_sum / TRIALS
                  << " | mean_distance="
                  << distance_sum / TRIALS
                  << '\n';
      }

      std::cout << '\n';
    }

    std::cout << "Experiment 001C completed.\n";
    std::cout << "Results: "
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
