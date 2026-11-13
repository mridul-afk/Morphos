
#include <gtest/gtest.h>

#include "morphos/cell.hpp"

#include <cstdint>
#include <stdexcept>

using morphos::Cell;

TEST(LineageTest, NewCellsStartUnassigned)
{
  Cell cell(10.0f, 20.0f);

  EXPECT_EQ(cell.id(), 0u);
  EXPECT_EQ(cell.parent_id(), 0u);
  EXPECT_EQ(cell.generation(), 0u);
}

TEST(LineageTest, AssignsFounderIdentity)
{
  Cell cell(10.0f, 20.0f);

  cell.set_lineage(1, 0, 0);

  EXPECT_EQ(cell.id(), 1u);
  EXPECT_EQ(cell.parent_id(), 0u);
  EXPECT_EQ(cell.generation(), 0u);
}

TEST(LineageTest, AssignsOffspringIdentity)
{
  Cell cell(10.0f, 20.0f);

  cell.set_lineage(5, 2, 1);

  EXPECT_EQ(cell.id(), 5u);
  EXPECT_EQ(cell.parent_id(), 2u);
  EXPECT_EQ(cell.generation(), 1u);
}

TEST(LineageTest, SupportsMultipleGenerations)
{
  Cell cell(10.0f, 20.0f);

  cell.set_lineage(100, 45, 12);

  EXPECT_EQ(cell.id(), 100u);
  EXPECT_EQ(cell.parent_id(), 45u);
  EXPECT_EQ(cell.generation(), 12u);
}

TEST(LineageTest, RejectsInvalidIdentity)
{
  Cell cell(10.0f, 20.0f);

  EXPECT_THROW(
      cell.set_lineage(0, 0, 0),
      std::invalid_argument);

  EXPECT_THROW(
      cell.set_lineage(5, 5, 1),
      std::invalid_argument);

  EXPECT_THROW(
      cell.set_lineage(5, 2, 0),
      std::invalid_argument);

  EXPECT_THROW(
      cell.set_lineage(5, 0, 1),
      std::invalid_argument);
}

TEST(LineageTest, InvalidAssignmentPreservesIdentity)
{
  Cell cell(10.0f, 20.0f);

  cell.set_lineage(7, 3, 2);

  EXPECT_THROW(
      cell.set_lineage(0, 0, 0),
      std::invalid_argument);

  EXPECT_EQ(cell.id(), 7u);
  EXPECT_EQ(cell.parent_id(), 3u);
  EXPECT_EQ(cell.generation(), 2u);
}
