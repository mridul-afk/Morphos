
#include "morphos/simulation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

using morphos::Simulation;
using morphos::SimulationConfig;

namespace
{

  constexpr int TRIALS = 30;
  constexpr int STEPS = 500;
  constexpr std::size_t POPULATION = 100;
  constexpr float DT = 1.0f;
  constexpr std::uint32_t BASE_SEED = 42;

  constexpr std::array<float, 6> SPEEDS = {
      0.0f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f};

  constexpr std::array<float, 4> COSTS = {
      0.01f, 0.05f, 0.1f, 0.5f};

  struct Metrics
  {
    std::size_t living = 0;
    double survival = 0.0;
    double mean_living_energy = 0.0;
    double total_energy = 0.0;
    double total_distance = 0.0;
    double mean_distance = 0.0;
  };

  Metrics measure(const Simulation &sim)
  {
    Metrics m;
    double living_energy = 0.0;

    for (const auto &cell : sim.cells())
    {
      m.total_energy += cell.energy();
      m.total_distance += cell.distance_traveled();

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
      m.mean_distance = m.total_distance / n;
    }

    if (m.living > 0)
    {
      m.mean_living_energy = living_energy / m.living;
    }

    return m;
  }

  SimulationConfig make_config(
      float speed,
      float cost,
      std::uint32_t seed)
  {
    SimulationConfig config;

    config.world_width = 100.0f;
    config.world_height = 100.0f;
    config.grid_width = 128;
    config.grid_height = 128;

    config.initial_resource_capacity = 100.0f;
    config.resource_regeneration_rate = 0.01f;

    config.initial_cell_energy = 10.0f;
    config.absorption_rate = 2.0f;
    config.metabolism_rate = 1.0f;

    config.movement_speed = speed;
    config.movement_cost = cost;
    config.seed = seed;

    return config;
  }

  void write_row(
      std::ofstream &out,
      int trial,
      std::uint32_t seed,
      float speed,
      float cost,
      int step,
      const Metrics &m)
  {
    out << trial << ','
        << seed << ','
        << speed << ','
        << cost << ','
        << step << ','
        << m.living << ','
        << m.survival << ','
        << m.mean_living_energy << ','
        << m.total_energy << ','
        << m.total_distance << ','
        << m.mean_distance << '\n';
  }

} // namespace

int main(int argc, char **argv)
{
  try
  {
    fs::path output_dir =
        argc > 1
            ? fs::path(argv[1])
            : fs::path(
                  "experiments/001_selection/results_001B");

    fs::create_directories(output_dir);

    std::ofstream summary(
        output_dir / "sensitivity_summary.csv");

    std::ofstream trajectories(
        output_dir / "sensitivity_trajectories.csv");

    if (!summary || !trajectories)
    {
      throw std::runtime_error(
          "Cannot open experiment output files");
    }

    summary << std::setprecision(10);
    trajectories << std::setprecision(10);

    const char *header =
        "trial,seed,speed,cost,step,living,"
        "survival_percent,mean_living_energy,"
        "total_energy,total_distance,mean_distance\n";

    summary << header;
    trajectories << header;

    std::cout << "MORPHOS Experiment 001B\n";
    std::cout << "Movement-energy sensitivity analysis\n";
    std::cout << "Configurations: "
              << SPEEDS.size() * COSTS.size()
              << "\nTrials/config: " << TRIALS
              << "\nSteps/trial: " << STEPS
              << "\n\n";

    int configuration = 0;

    for (float cost : COSTS)
    {
      for (float speed : SPEEDS)
      {
        ++configuration;

        double survival_sum = 0.0;
        double distance_sum = 0.0;
        double energy_sum = 0.0;

        for (int trial = 0; trial < TRIALS; ++trial)
        {
          const std::uint32_t seed =
              BASE_SEED +
              static_cast<std::uint32_t>(trial);

          Simulation sim(
              make_config(speed, cost, seed));

          sim.initialize(POPULATION);

          write_row(
              trajectories,
              trial + 1, seed,
              speed, cost, 0, measure(sim));

          for (int step = 1;
               step <= STEPS;
               ++step)
          {
            sim.step(DT);

            write_row(
                trajectories,
                trial + 1, seed,
                speed, cost, step,
                measure(sim));
          }

          const Metrics final = measure(sim);

          write_row(
              summary,
              trial + 1, seed,
              speed, cost, STEPS, final);

          survival_sum += final.survival;
          distance_sum += final.mean_distance;
          energy_sum += final.total_energy;
        }

        std::cout
            << "[" << configuration << "/"
            << SPEEDS.size() * COSTS.size()
            << "] speed=" << speed
            << " cost=" << cost
            << " survival=" << std::fixed
            << std::setprecision(2)
            << survival_sum / TRIALS << "%"
            << " mean_distance="
            << distance_sum / TRIALS
            << " total_energy="
            << energy_sum / TRIALS
            << '\n';
      }
    }

    std::cout << "\nExperiment 001B completed.\n";
    std::cout << "Output: "
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
