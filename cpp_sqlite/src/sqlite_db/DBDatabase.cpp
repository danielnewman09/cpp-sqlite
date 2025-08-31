#include "sqlite_db/DBDatabase.hpp"
#include <stdexcept>

namespace cpp_sqlite
{

Database::Database(std::string url,
                   bool allowWrite,
                   std::shared_ptr<spdlog::logger> pLogger)
  : db_(nullptr, sqlite3_close), pLogger_{pLogger}
{
  if (pLogger_)
  {
    pLogger->debug("Creating database with url: {}", url);
  }

  sqlite3* raw_db = nullptr;

  // Determine flags based on allowWrite parameter
  int flags = allowWrite ? (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)
                         : SQLITE_OPEN_READONLY;

  // Open the database
  int result = sqlite3_open_v2(url.c_str(), &raw_db, flags, nullptr);

  if (result != SQLITE_OK)
  {
    std::string error_msg = "Failed to open database: ";
    if (raw_db)
    {
      error_msg += sqlite3_errmsg(raw_db);
      sqlite3_close(raw_db);
    }
    else
    {
      error_msg += "Unknown error";
    }
    throw std::runtime_error(error_msg);
  }

  // Transfer ownership to unique_ptr
  db_.reset(raw_db);
}

sqlite3* Database::getRawDB()
{
  return db_.get();
}

}  // namespace cpp_sqlite