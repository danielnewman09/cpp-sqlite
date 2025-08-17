#ifndef DATA_ACCESS_OBJECT_HPP
#define DATA_ACCESS_OBJECT_HPP

#include <vector>

#include "sqlite_db/DBDatabase.hpp"
#include "sqlite_db/DBTraits.hpp"

namespace cpp_sqlite
{

template <ValidTransferObject T>
class DataAccessObject
{
public:
  /*!
   * Construct a data access object for this
   * database
   */
  DataAccessObject(db& database)
    : tableName_{boost::typeindex::type_id<T>().pretty_name()},
      insertStmt_{nullptr, sqlite3_finalize},
      db_{db},
      dataBuffer_{}
  {
  }

  /*!
   * \brief Perform an insert with the buffer data
   */
  void insert()
  {
    db_.insert(insertStmt_, dataBuffer_);
  }

private:
  //! The name of the table accessed by this object.
  std::string tableName_;

  //!< The prepared statement to facilitate inserting
  //!< data into the database
  PreparedSQLStmt insertStmt_;

  //!< The reference to the database used by this
  //!< Data Access Object
  Database& db_;

  //! The internal buffer used to facilitate read/write
  //! to the database.
  std::vector<T> dataBuffer_;
};

}  // namespace cpp_sqlite

#endif  // DATA_ACCESS_OBJECT_HPP
