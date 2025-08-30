#include <stdexcept>
#include <string>

#include <boost/describe.hpp>
#include <boost/describe/class.hpp>

#include "cpp_sqlite/test/testDatabase.hpp"
#include "sqlite_db/DBDatabase.hpp"
#include "sqlite_db/DBBaseTransferObject.hpp"
#include "sqlite_db/DBDataAccessObject.hpp"

// Test TransferObject class for demonstration
struct TestProduct : public cpp_sqlite::BaseTransferObject
{
  std::string name;
  float price;
  int quantity;
  bool in_stock;
};

// Register the test class with boost::describe
BOOST_DESCRIBE_STRUCT(TestProduct, (cpp_sqlite::BaseTransferObject), (name, price, quantity, in_stock));

void DatabaseTest::SetUp()
{
  // Configure logger for testing with debug level
  cpp_sqlite::Logger::getInstance().configure(
    "test_cpp_sqlite", testLogFile, spdlog::level::debug);
}

void DatabaseTest::TearDown()
{
  // Clean up any test.db file that might exist
  if (std::filesystem::exists("test.db"))
  {
    std::filesystem::remove("test.db");
  }
}

TEST_F(DatabaseTest, CreateInMemoryDatabase)
{
  std::string dbUrl{":memory:"};

  auto& logger = cpp_sqlite::Logger::getInstance();

  // Test creating an in-memory database
  auto createDB = [&dbUrl, &logger](bool readOnly)
  { return cpp_sqlite::Database{dbUrl, readOnly, logger.getLogger()}; };
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
  ASSERT_THROW(
    { cpp_sqlite::Database db(nonExistentFile, false); }, std::runtime_error);
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

TEST_F(DatabaseTest, BoostDescribeCreateTableGeneration)
{
  const std::string testDbFile = "test_boost_describe.db";

  // Ensure clean state
  if (std::filesystem::exists(testDbFile))
  {
    std::filesystem::remove(testDbFile);
  }

  try
  {
    // Get logger instance
    auto& logger = cpp_sqlite::Logger::getInstance();
    
    // Create database
    cpp_sqlite::Database db{testDbFile, true, logger.getLogger()};
    
    // Create DataAccessObject for our test TransferObject
    cpp_sqlite::DataAccessObject<TestProduct> productDAO(db);
    
    // Execute the create statement - this should generate:
    // CREATE TABLE IF NOT EXISTS TestProduct (id INTEGER PRIMARY KEY, name TEXT, price REAL, quantity INTEGER, in_stock INTEGER);
    bool createResult = productDAO.executeCreateStmt();
    
    // Verify table creation succeeded
    ASSERT_TRUE(createResult) << "Failed to create table using boost::describe";
    
    // Verify the table actually exists by querying SQLite's metadata
    sqlite3* rawDb = db.getRawDB();
    sqlite3_stmt* stmt = nullptr;
    
    const char* checkTableQuery = "SELECT name FROM sqlite_master WHERE type='table' AND name='TestProduct';";
    int result = sqlite3_prepare_v2(rawDb, checkTableQuery, -1, &stmt, nullptr);
    ASSERT_EQ(result, SQLITE_OK) << "Failed to prepare table existence query";
    
    result = sqlite3_step(stmt);
    ASSERT_EQ(result, SQLITE_ROW) << "Table 'TestProduct' was not created";
    
    // Verify the table name
    const char* tableName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    ASSERT_STREQ(tableName, "TestProduct") << "Table name doesn't match expected";
    
    sqlite3_finalize(stmt);
    
    // Optionally verify the table structure
    const char* pragmaQuery = "PRAGMA table_info(TestProduct);";
    result = sqlite3_prepare_v2(rawDb, pragmaQuery, -1, &stmt, nullptr);
    ASSERT_EQ(result, SQLITE_OK) << "Failed to prepare PRAGMA query";
    
    // Count columns (should have 5: id, name, price, quantity, in_stock)
    int columnCount = 0;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
    {
      columnCount++;
      
      // Get column info for verification
      const char* colName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      const char* colType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
      
      // Log column info for debugging
      auto& logger = cpp_sqlite::Logger::getInstance();
      if (logger.isConfigured())
      {
        logger.getLogger()->debug("Column: {} - Type: {}", colName, colType);
      }
    }
    
    sqlite3_finalize(stmt);
    
    // Verify we have the expected number of columns
    ASSERT_EQ(columnCount, 5) << "Expected 5 columns (id + 4 members), got " << columnCount;
    
    // Log success
    auto& logger = cpp_sqlite::Logger::getInstance();
    if (logger.isConfigured())
    {
      logger.getLogger()->info("Successfully demonstrated boost::describe table creation for TestProduct");
    }
  }
  catch (const std::exception& ex)
  {
    FAIL() << "Exception during boost::describe test: " << ex.what();
  }

  // Clean up
  if (std::filesystem::exists(testDbFile))
  {
    std::filesystem::remove(testDbFile);
  }
}