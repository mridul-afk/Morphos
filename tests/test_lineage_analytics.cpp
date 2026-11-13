
#include <gtest/gtest.h>

#include "morphos/lineage_analytics.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

using morphos::LineageAnalytics;
using morphos::LineageRecord;

namespace
{
  LineageRecord make_record(
      std::uint64_t id,
      std::uint64_t parent,
      std::uint64_t generation,
      bool alive = true)
  {
    LineageRecord r;
    r.id = id;
    r.parent_id = parent;
    r.generation = generation;
    r.alive = alive;
    return r;
  }
}

TEST(LineageAnalyticsTest, EmptyHistory)
{
  const std::vector<LineageRecord> records;

  EXPECT_TRUE(
      LineageAnalytics::analyze(records).empty());

  EXPECT_EQ(
      LineageAnalytics::maximum_generation(records),
      0u);
}

TEST(LineageAnalyticsTest, FounderWithoutChildren)
{
  const std::vector<LineageRecord> records = {
      make_record(1, 0, 0)};

  const auto stats = LineageAnalytics::analyze(records);

  ASSERT_EQ(stats.size(), 1u);
  EXPECT_EQ(stats[0].founder_id, 1u);
  EXPECT_EQ(stats[0].total_descendants, 0u);
  EXPECT_EQ(stats[0].living_descendants, 0u);
  EXPECT_TRUE(stats[0].lineage_survives);
}

TEST(LineageAnalyticsTest, CountsDescendants)
{
  // 1 -> 2 -> 4
  //   -> 3
  const std::vector<LineageRecord> records = {
      make_record(1, 0, 0),
      make_record(2, 1, 1),
      make_record(3, 1, 1),
      make_record(4, 2, 2)};

  const auto stats = LineageAnalytics::analyze(records);

  ASSERT_EQ(stats.size(), 1u);
  EXPECT_EQ(stats[0].total_descendants, 3u);
  EXPECT_EQ(stats[0].living_descendants, 3u);
  EXPECT_EQ(stats[0].max_generation, 2u);
}

TEST(LineageAnalyticsTest, SeparatesFounderLineages)
{
  const std::vector<LineageRecord> records = {
      make_record(1, 0, 0),
      make_record(2, 0, 0),
      make_record(3, 1, 1),
      make_record(4, 3, 2),
      make_record(5, 2, 1)};

  const auto stats = LineageAnalytics::analyze(records);

  ASSERT_EQ(stats.size(), 2u);

  EXPECT_EQ(stats[0].founder_id, 1u);
  EXPECT_EQ(stats[0].total_descendants, 2u);

  EXPECT_EQ(stats[1].founder_id, 2u);
  EXPECT_EQ(stats[1].total_descendants, 1u);
}

TEST(LineageAnalyticsTest, DetectsExtinctLineage)
{
  const std::vector<LineageRecord> records = {
      make_record(1, 0, 0, false),
      make_record(2, 1, 1, false)};

  const auto stats = LineageAnalytics::analyze(records);

  ASSERT_EQ(stats.size(), 1u);

  EXPECT_FALSE(stats[0].founder_alive);
  EXPECT_FALSE(stats[0].lineage_survives);
  EXPECT_EQ(stats[0].living_descendants, 0u);
}

TEST(LineageAnalyticsTest, DescendantsCanOutliveFounder)
{
  const std::vector<LineageRecord> records = {
      make_record(1, 0, 0, false),
      make_record(2, 1, 1, true)};

  const auto stats = LineageAnalytics::analyze(records);

  ASSERT_EQ(stats.size(), 1u);

  EXPECT_FALSE(stats[0].founder_alive);
  EXPECT_TRUE(stats[0].lineage_survives);
  EXPECT_EQ(stats[0].living_descendants, 1u);
}

TEST(LineageAnalyticsTest, RejectsMissingParent)
{
  const std::vector<LineageRecord> records = {
      make_record(2, 99, 1)};

  EXPECT_THROW(
      LineageAnalytics::analyze(records),
      std::invalid_argument);
}

TEST(LineageAnalyticsTest, RejectsDuplicateIds)
{
  const std::vector<LineageRecord> records = {
      make_record(1, 0, 0),
      make_record(1, 0, 0)};

  EXPECT_THROW(
      LineageAnalytics::analyze(records),
      std::invalid_argument);
}

TEST(LineageAnalyticsTest, RejectsCycles)
{
  const std::vector<LineageRecord> records = {
      make_record(1, 2, 1),
      make_record(2, 1, 2)};

  EXPECT_THROW(
      LineageAnalytics::analyze(records),
      std::invalid_argument);
}

TEST(LineageAnalyticsTest, ExportsCsv)
{
  const auto path =
      std::filesystem::temp_directory_path() /
      "morphos_lineage_test.csv";

  const std::vector<LineageRecord> records = {
      make_record(1, 0, 0),
      make_record(2, 1, 1)};

  LineageAnalytics::export_csv(records, path);

  std::ifstream file(path);
  ASSERT_TRUE(file.is_open());

  const std::string content(
      (std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>());

  EXPECT_NE(
      content.find("parent_id"),
      std::string::npos);

  EXPECT_NE(
      content.find("2,1,1"),
      std::string::npos);

  file.close();
  std::filesystem::remove(path);
}

TEST(LineageAnalyticsTest, ExportsSummaryCsv)
{
  const auto path =
      std::filesystem::temp_directory_path() /
      "morphos_lineage_summary_test.csv";

  const std::vector<LineageRecord> records = {
      make_record(1, 0, 0),
      make_record(2, 1, 1)};

  const auto stats =
      LineageAnalytics::analyze(records);

  LineageAnalytics::export_summary_csv(stats, path);

  std::ifstream file(path);
  ASSERT_TRUE(file.is_open());

  const std::string content(
      (std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>());

  EXPECT_NE(
      content.find("total_descendants"),
      std::string::npos);

  EXPECT_NE(
      content.find("1,1,1,1,1,1"),
      std::string::npos);

  file.close();
  std::filesystem::remove(path);
}
