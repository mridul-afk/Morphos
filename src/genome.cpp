#include "morphos/genome.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace morphos
{

    void Genome::validate() const
    {
        if (!std::isfinite(movement_speed) ||
            !std::isfinite(absorption_rate) ||
            movement_speed < 0.0f ||
            absorption_rate < 0.0f)
        {
            throw std::invalid_argument(
                "Genome contains an invalid gene value");
        }
    }

    Genome Genome::mutated(
        std::mt19937 &rng,
        float mutation_probability,
        float mutation_strength) const
    {
        validate();

        if (!std::isfinite(mutation_probability) ||
            mutation_probability < 0.0f ||
            mutation_probability > 1.0f)
        {
            throw std::invalid_argument(
                "Mutation probability must be between 0 and 1");
        }

        if (!std::isfinite(mutation_strength) ||
            mutation_strength < 0.0f)
        {
            throw std::invalid_argument(
                "Mutation strength must be nonnegative");
        }

        Genome child = *this;

        std::bernoulli_distribution should_mutate(
            mutation_probability);

        std::normal_distribution<float> noise(
            0.0f, mutation_strength);

        if (should_mutate(rng))
        {
            const float candidate =
                child.movement_speed + noise(rng);

            if (std::isfinite(candidate))
            {
                child.movement_speed =
                    std::max(0.0f, candidate);
            }
        }

        if (should_mutate(rng))
        {
            const float candidate =
                child.absorption_rate + noise(rng);

            if (std::isfinite(candidate))
            {
                child.absorption_rate =
                    std::max(0.0f, candidate);
            }
        }

        child.validate();

        return child;
    }

}
