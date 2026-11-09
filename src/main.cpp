
#include <iostream>

#include "morphos/simulation.hpp"

int main()
{
    morphos::Simulation simulation(
        1024.0f,
        1024.0f,
        42);

    simulation.initialize(100);

    std::cout << "================================\n";
    std::cout << "          MORPHOS v0.1         \n";
    std::cout << "================================\n\n";

    std::cout << "Initial population: "
              << simulation.living_population() << '\n';

    for (int step = 0; step < 100; ++step)
    {
        simulation.step(1.0f);

        if ((step + 1) % 10 == 0)
        {
            std::cout
                << "Step " << simulation.timestep()
                << " | Living cells: "
                << simulation.living_population()
                << '\n';
        }
    }

    std::cout << "\nSimulation completed.\n";

    return 0;
}
