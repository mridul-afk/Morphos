#include "morphos/lineage_analytics.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace morphos
{
  namespace
  {
    using RecordMap =
        std::unordered_map<
            std::uint64_t,
            const LineageRecord *>;

    RecordMap build_index(
        const std::vector<LineageRecord> &records)
    {
      RecordMap index;
      index.reserve(records.size());

      for (const auto &record : records)
      {
        if (record.id == 0)
        {
          throw std::invalid_argument(
              "Lineage contains an unassigned ID");
        }

        if (!index.emplace(
                      record.id, &record)
                 .second)
        {
          throw std::invalid_argument(
              "Duplicate lineage ID");
        }
      }

      return index;
    }

    std::ofstream open_csv(
        const std::filesystem::path &path)
    {
      std::ofstream out(
          path,
          std::ios::out | std::ios::trunc);

      if (!out.is_open())
      {
        throw std::runtime_error(
            "Unable to open lineage CSV output");
      }

      return out;
    }

    void check_output(std::ofstream &out)
    {
      out.flush();

      if (!out)
      {
        throw std::runtime_error(
            "Failed writing lineage CSV");
      }
    }
  }

  std::vector<LineageStats>
  LineageAnalytics::analyze(
      const std::vector<LineageRecord> &records)
  {
    const RecordMap index = build_index(records);

    // Cache organism ID -> founding organism ID.
    std::unordered_map<
        std::uint64_t,
        std::uint64_t>
        founder_cache;

    founder_cache.reserve(records.size());

    // Tracks ancestry currently being resolved.
    std::unordered_set<std::uint64_t> visiting;

    std::function<std::uint64_t(std::uint64_t)>
        find_founder;

    find_founder = [&](std::uint64_t id)
        -> std::uint64_t
    {
      const auto cached = founder_cache.find(id);

      if (cached != founder_cache.end())
      {
        return cached->second;
      }

      const auto it = index.find(id);

      if (it == index.end())
      {
        throw std::invalid_argument(
            "Missing ancestor in lineage history");
      }

      if (!visiting.insert(id).second)
      {
        throw std::invalid_argument(
            "Cycle detected in lineage history");
      }

      const LineageRecord &record = *it->second;

      std::uint64_t founder = 0;

      if (record.parent_id == 0)
      {
        if (record.generation != 0)
        {
          throw std::invalid_argument(
              "Founder has nonzero generation");
        }

        founder = record.id;
      }
      else
      {
        const auto parent_it =
            index.find(record.parent_id);

        if (parent_it == index.end())
        {
          throw std::invalid_argument(
              "Parent record is missing");
        }

        const LineageRecord &parent =
            *parent_it->second;

        if (record.generation == 0 ||
            record.generation - 1 !=
                parent.generation ||
            record.birth_timestep <
                parent.birth_timestep)
        {
          throw std::invalid_argument(
              "Invalid lineage generation or chronology");
        }

        founder = find_founder(record.parent_id);
      }

      visiting.erase(id);
      founder_cache.emplace(id, founder);

      return founder;
    };

    // Resolve every organism and validate ancestry.
    for (const auto &record : records)
    {
      find_founder(record.id);
    }

    std::vector<LineageStats> result;

    std::unordered_map<
        std::uint64_t,
        std::size_t>
        stats_index;

    // Create one statistics entry per founder.
    for (const auto &record : records)
    {
      if (record.parent_id != 0)
      {
        continue;
      }

      LineageStats stats;
      stats.founder_id = record.id;
      stats.founder_alive = record.alive;
      stats.lineage_survives = record.alive;

      stats_index.emplace(
          record.id, result.size());

      result.push_back(stats);
    }

    // Accumulate descendants for each founder.
    for (const auto &record : records)
    {
      const std::uint64_t founder =
          founder_cache.at(record.id);

      LineageStats &stats =
          result.at(stats_index.at(founder));

      stats.max_generation = std::max(
          stats.max_generation,
          record.generation);

      // Founder is not its own descendant.
      if (record.id == founder)
      {
        continue;
      }

      ++stats.total_descendants;

      if (record.alive)
      {
        ++stats.living_descendants;
        stats.lineage_survives = true;
      }
    }

    std::sort(
        result.begin(),
        result.end(),
        [](const LineageStats &a,
           const LineageStats &b)
        {
          return a.founder_id < b.founder_id;
        });

    return result;
  }

  std::uint64_t
  LineageAnalytics::maximum_generation(
      const std::vector<LineageRecord> &records)
  {
    std::uint64_t maximum = 0;

    for (const auto &record : records)
    {
      maximum = std::max(
          maximum,
          record.generation);
    }

    return maximum;
  }

  void LineageAnalytics::export_csv(
      const std::vector<LineageRecord> &records,
      const std::filesystem::path &path)
  {
    auto out = open_csv(path);

    out << "id,parent_id,generation,"
           "birth_timestep,death_timestep,"
           "alive,movement_speed,absorption_rate\n";

    for (const auto &record : records)
    {
      out << record.id << ','
          << record.parent_id << ','
          << record.generation << ','
          << record.birth_timestep << ','
          << record.death_timestep << ','
          << (record.alive ? 1 : 0) << ','
          << record.birth_genome.movement_speed << ','
          << record.birth_genome.absorption_rate
          << '\n';
    }

    check_output(out);
  }

  void LineageAnalytics::export_summary_csv(
      const std::vector<LineageStats> &stats,
      const std::filesystem::path &path)
  {
    auto out = open_csv(path);

    out << "founder_id,total_descendants,"
           "living_descendants,max_generation,"
           "founder_alive,lineage_survives\n";

    for (const auto &stat : stats)
    {
      out << stat.founder_id << ','
          << stat.total_descendants << ','
          << stat.living_descendants << ','
          << stat.max_generation << ','
          << (stat.founder_alive ? 1 : 0) << ','
          << (stat.lineage_survives ? 1 : 0)
          << '\n';
    }

    check_output(out);
  }

}
