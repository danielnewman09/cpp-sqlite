#include <gtest/gtest.h>

#include "sqlite_db/example.hpp"  // Include the header for the code we want to test

// Test fixture for the 'add' function
TEST(AddTest, PositiveNumbers)
{
  ASSERT_EQ(add(2, 3), 5);
}

TEST(AddTest, NegativeNumbers)
{
  ASSERT_EQ(add(-2, -3), -5);
}