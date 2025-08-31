
#ifndef DB_DATABASE_HPP
#define DB_DATABASE_HPP

#include <memory>
#include <string>
#include <vector>

#include "sqlite3.h"

#include "Logger.hpp"
#include "sqlite_db/DBBaseTransferObject.hpp"
#include "sqlite_db/DBTraits.hpp"

namespace cpp_sqlite
{
/*!
 * A wrapping alias for the sqlite3 prepared statement
 * that allows us to use modern C++ memory management
 * with this library.
 */
using PreparedSQLStmt =
  std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

class Database
{
public:
  /*!
   * \brief Create a SQLite database with a given
   *        url and specify whether we allow read
   *        and write operations
   * \param url The string url to pass to the
   *        sqlite constructor
   * \param allowWrite The boolean indicating
   *        whether this is a read-only database.
   */
  Database(std::string url,
           bool allowWrite,
           std::shared_ptr<spdlog::logger> pLogger = nullptr);

  /*!
   * \brief Perform a generic insert operation
   */
  template <ValidTransferObject T>
  bool insert(PreparedSQLStmt& stmt, const std::vector<T>& data)
  {
    // Implementation placeholder - method signature now functional
    return false;
  }

  /*!
   * \brief Get raw SQLite database pointer for direct access
   * \return Raw sqlite3* pointer
   */
  sqlite3* getRawDB();

private:
  //!< The unique pointer storing the SQLite database
  //!< object
  std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db_;

  //! The pointer to the spdlog for this object.
  std::shared_ptr<spdlog::logger> pLogger_;
};

}  // namespace cpp_sqlite

#endif  // DB_DATABASE_HPP