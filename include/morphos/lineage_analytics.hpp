#pragma once

#include "morphos/lineage.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace morphos
{
  struct LineageStats
  {
    std::uint64_t founder_id = 0;

    std::size_t total_descendants = 0;
    std::size_t living_descendants = 0;

    std::uint64_t max_generation = 0;

    bool founder_alive = false;
    bool lineage_survives = false;
  };

  class LineageAnalytics
  {
  public:
    // One summary per founding organism.
    [[nodiscard]] static std::vector<LineageStats>
    analyze(
        const std::vector<LineageRecord> &records);

    // Maximum generation across all organisms.
    [[nodiscard]] static std::uint64_t
    maximum_generation(
        const std::vector<LineageRecord> &records);

    // Export all recorded organisms.
    static void export_csv(
        const std::vector<LineageRecord> &records,
        const std::filesystem::path &path);

    // Export one row per founding lineage.
    static void export_summary_csv(
        const std::vector<LineageStats> &stats,
        const std::filesystem::path &path);
  };
}
