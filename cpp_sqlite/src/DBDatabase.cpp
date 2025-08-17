#include "cpp_sqlite/src/DBDatabase.hpp"
#include <stdexcept>

namespace cpp_sqlite
{

Database::Database(std::string_view url, bool allowWrite) : db_(nullptr, sqlite3_close)
{
  sqlite3* raw_db = nullptr;

  // Determine flags based on allowWrite parameter
  int flags = allowWrite ? (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) : SQLITE_OPEN_READONLY;

  // Open the database
  int result = sqlite3_open_v2(url.data(), &raw_db, flags, nullptr);

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

}  // namespace cpp_sqlite