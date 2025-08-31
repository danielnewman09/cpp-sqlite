#ifndef DATA_ACCESS_OBJECT_HPP
#define DATA_ACCESS_OBJECT_HPP

#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

#include <boost/describe.hpp>
#include <boost/describe/class.hpp>
#include <boost/mp11.hpp>
#include <boost/type_index.hpp>

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
  DataAccessObject(Database& database)
    : tableName_{boost::typeindex::type_id<T>().pretty_name()},
      insertStmt_{nullptr, sqlite3_finalize},
      createStmt_{nullptr, sqlite3_finalize},
      db_{database},
      dataBuffer_{},
      isInitialized_{true}
  {
    isInitialized_ = executeCreateStmt();
  }

  /*!
   * \brief Perform an insert with the buffer data
   */
  void insert()
  {
    db_.insert(insertStmt_, dataBuffer_);
  }

  /*!
   * \brief Get the initialization status of this DAO
   * \return The initialization status of the object.
   */
  bool isInitialized() const
  {
    return isInitialized_;
  }

private:
  // Helper function to map C++ types to SQL types
  template <isSupportedDBType FieldType>
  constexpr const char* getSQLType()
  {
    if constexpr (isIntegral<FieldType>)
      return "INTEGER";
    else if constexpr (floatingPoint<FieldType>)
      return "REAL";
    else if constexpr (std::is_same_v<FieldType, std::string>)
      return "TEXT";
    else
      return "BLOB";  // Default for unknown types
  }

  // Generate CREATE TABLE SQL using boost::describe
  std::string generateCreateTableSQL()
  {
    std::ostringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS " << tableName_ << " (";

    bool first = true;

    // Process public members
    boost::mp11::mp_for_each<boost::describe::describe_members<
      T,
      boost::describe::mod_inherited | boost::describe::mod_public>>(
      [&](auto D)
      {
        if (!first)
          sql << ", ";

        // Get member name
        sql << D.name;

        // Get member type and map to SQL type
        using member_type = std::remove_cv_t<
          std::remove_reference_t<decltype(std::declval<T>().*D.pointer)>>;
        sql << " " << getSQLType<member_type>();

        // Add PRIMARY KEY for id field
        if (std::string(D.name) == "id")
        {
          sql << " PRIMARY KEY";
        }

        first = false;
      });

    sql << ");";
    return sql.str();
  }

  bool prepareSQLStatements()
  {
    return false;
  }

  bool executeCreateStmt()
  {
    std::string createQuery = generateCreateTableSQL();

    sqlite3_stmt* rawPtr = nullptr;
    int result = sqlite3_prepare_v2(
      db_.getRawDB(), createQuery.c_str(), -1, &rawPtr, nullptr);

    if (result != SQLITE_OK)
    {
      return false;
    }

    result = sqlite3_step(rawPtr);
    sqlite3_finalize(rawPtr);

    return (result == SQLITE_DONE);
  }

  //! The name of the table accessed by this object.
  std::string tableName_;

  //!< The prepared statement to facilitate inserting
  //!< data into the database
  PreparedSQLStmt insertStmt_;

  //!<
  PreparedSQLStmt createStmt_;

  //!< The reference to the database used by this
  //!< Data Access Object
  Database& db_;

  //! The internal buffer used to facilitate read/write
  //! to the database.
  std::vector<T> dataBuffer_;

  //! Tracks whether or not the DAO is initialized
  bool isInitialized_;
};

}  // namespace cpp_sqlite

#endif  // DATA_ACCESS_OBJECT_HPP
