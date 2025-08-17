#include <stdexcept>

#include "cpp_sqlite/test/testDatabase.hpp"
#include "sqlite_db/DBDatabase.hpp"

TEST_F(DatabaseTest, CreateInMemoryDatabase)
{
  std::string dbUrl{":memory:"};

  // Test creating an in-memory database
  auto createDB = [&dbUrl](bool readOnly) { return cpp_sqlite::Database{dbUrl, readOnly}; };
  ASSERT_NO_THROW(createDB(true););

  // Test both read-write and read-only in-memory databases
  ASSERT_NO_THROW({
    createDB(true);
    createDB(false);
  });
}

TEST_F(DatabaseTest, ReadOnlyNonExistentFileThrowsError)
{
  // Ensure the file doesn't exist
  const std::string nonExistentFile = "non_existent_database.db";
  if (std::filesystem::exists(nonExistentFile))
  {
    std::filesystem::remove(nonExistentFile);
  }

  // Attempting to open a non-existent file in read-only mode should throw
  ASSERT_THROW({ cpp_sqlite::Database db(nonExistentFile, false); }, std::runtime_error);
}

TEST_F(DatabaseTest, CreateFileDatabase)
{
  const std::string testDbFile = "test.db";

  // Ensure clean state
  if (std::filesystem::exists(testDbFile))
  {
    std::filesystem::remove(testDbFile);
  }

  // Create database file
  {
    cpp_sqlite::Database db{testDbFile, true};
    // Database should be created successfully
    ASSERT_TRUE(std::filesystem::exists(testDbFile));
  }

  // Verify we can open it in read-only mode now that it exists
  ASSERT_NO_THROW({ cpp_sqlite::Database db_readonly(testDbFile, false); });

  // Clean up - file should be deleted
  ASSERT_TRUE(std::filesystem::exists(testDbFile));
  std::filesystem::remove(testDbFile);
  ASSERT_FALSE(std::filesystem::exists(testDbFile));
}