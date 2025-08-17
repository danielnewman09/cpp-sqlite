#ifndef DATABASE_TEST_HPP
#define DATABASE_TEST_HPP

#include <gtest/gtest.h>
#include <filesystem>


class DatabaseTest : public ::testing::Test
{
protected:
  void TearDown() override
  {
    // Clean up any test.db file that might exist
    if (std::filesystem::exists("test.db"))
    {
      std::filesystem::remove("test.db");
    }
  }
};

#endif  // DATABASE_TEST_HPP