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
#include "sqlite3.h"

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


/*!
 * Abstract base class for all Data Access Objects
 * Provides common interface for polymorphic storage
 */
class DAOBase
{
public:
  virtual ~DAOBase() = default;

  /*!
   * \brief Check if the DAO is properly initialized
   */
  virtual bool isInitialized() const = 0;

  /*!
   * \brief Perform insert operation with buffered data
   */
  virtual void insert() = 0;

  /*!
   * \brief Clear the internal data buffer
   */
  virtual void clearBuffer() = 0;
};

template <ValidTransferObject T>
class DataAccessObject : public DAOBase
{
public:
  /*!
   * Construct a data access object for this
   * database
   */
  DataAccessObject(sqlite3* database,
                   std::shared_ptr<spdlog::logger> pLogger = nullptr)
    : tableName_{boost::typeindex::type_id<T>().pretty_name()},
      insertStmt_{nullptr, sqlite3_finalize},
      createStmt_{nullptr, sqlite3_finalize},
      db_{database},
      dataBuffer_{},
      isInitialized_{true},
      pLogger_{pLogger}
  {
    isInitialized_ = executeCreateStmt();
    isInitialized_ &= prepareInsertStatement();
  }

  // Virtual methods from DAOBase
  /*!
   * \brief Get the initialization status of this DAO
   * \return The initialization status of the object.
   */
  bool isInitialized() const override
  {
    return isInitialized_;
  }

  /*!
   * \brief Perform an insert with the buffer data
   */
  void insert() override
  {
    if (!insertStmt_)
    {
      return;  // No prepared statement available
    }

    for (const auto& obj : dataBuffer_)
    {
      // Reset the statement for reuse
      sqlite3_reset(insertStmt_.get());

      // Bind parameters and execute
      // Note: This is a simplified implementation
      // In a real implementation, you'd bind each field of obj to the statement
      int result = sqlite3_step(insertStmt_.get());

      if (result != SQLITE_DONE)
      {
        LOG_SAFE(
          pLogger_, spdlog::level::err, "Insert failed with code: {}", result);
      }
    }
  }

  /*!
   * \brief Clear the data buffer
   */
  void clearBuffer() override
  {
    dataBuffer_.clear();
  }

  // Type-specific non-virtual methods
  /*!
   * \brief Add object to buffer for insertion
   */
  void addToBuffer(const T& obj)
  {
    dataBuffer_.push_back(obj);
  }

private:
  // Helper function to map C++ types to SQL types
  template <isSupportedDBType FieldType>
  constexpr const char* getSQLType()
  {
    if constexpr (isIntegral<FieldType>)
    {
      return "INTEGER";
    }
    else if constexpr (floatingPoint<FieldType>)
    {
      return "FLOAT";
    }
    else if constexpr (std::is_same_v<FieldType, std::string>)
    {
      return "TEXT";
    }
    else
    {
      return "BLOB";  // Default for unknown types
    }
  }

  /*!
   * \brief Create the string that is used to generate the SQL
   *        CREATE TABLE command
   * \return A string for the table creation command.
   */
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
        using memberType = std::remove_cv_t<
          std::remove_reference_t<decltype(std::declval<T>().*D.pointer)>>;

        // If the field is, itself, another transfer object, we treat
        // this field in the current table as a key to the table
        // that the child object presumably exists at.
        if constexpr (ValidTransferObject<memberType>)
        {
          using idType = decltype(std::declval<memberType>().id);
          sql << "_id" << " " << getSQLType<idType>();

          sql << ", FOREIGN KEY " << "(" << D.name << "_id) REFERENCES "
              << boost::typeindex::type_id<T>().pretty_name() << "(id)";
        }
        else
        {
          sql << " " << getSQLType<memberType>();

          // Add PRIMARY KEY for id field
          if (std::string(D.name) == "id")
          {
            sql << " PRIMARY KEY";
          }
        }


        first = false;
      });

    sql << ");";
    return sql.str();
  }

  bool prepareSQLStatements()
  {
    return prepareInsertStatement();
  }

  bool prepareInsertStatement()
  {
    std::string insertQuery = generateInsertSQL();

    LOG_SAFE(pLogger_, spdlog::level::debug, insertQuery);

    sqlite3_stmt* rawPtr = nullptr;
    int result =
      sqlite3_prepare_v2(db_, insertQuery.c_str(), -1, &rawPtr, nullptr);

    if (result != SQLITE_OK)
    {
      LOG_SAFE(
        pLogger_, spdlog::level::err, "Could not prepare insert statement");
      return false;
    }

    insertStmt_.reset(rawPtr);
    return true;
  }

  /*!
   * \brief Create the string that prepares an insert statement.
   *
   * \return The string for a prepared insert statement for a DB
   *         table.
   */
  std::string generateInsertSQL()
  {
    std::ostringstream sql;
    sql << "INSERT INTO " << tableName_ << " (";

    std::vector<std::string> columns;
    std::vector<std::string> placeholders;

    // Process public members to build column list
    boost::mp11::mp_for_each<boost::describe::describe_members<
      T,
      boost::describe::mod_inherited | boost::describe::mod_public>>(
      [&](auto D)
      {
        using memberType = std::remove_cv_t<
          std::remove_reference_t<decltype(std::declval<T>().*D.pointer)>>;

        // If the field is another transfer object, we use the foreign key
        // column name
        if constexpr (ValidTransferObject<memberType>)
        {
          columns.push_back(std::string(D.name) + "_id");
        }
        else
        {
          columns.push_back(std::string(D.name));
        }
        placeholders.push_back("?");
      });

    // Build the column names part
    bool first = true;
    for (const auto& column : columns)
    {
      if (!first)
        sql << ", ";
      sql << column;
      first = false;
    }

    sql << ") VALUES (";

    // Build the placeholders part
    first = true;
    for (const auto& placeholder : placeholders)
    {
      if (!first)
        sql << ", ";
      sql << placeholder;
      first = false;
    }

    sql << ");";
    return sql.str();
  }

  bool executeCreateStmt()
  {
    std::string createQuery = generateCreateTableSQL();

    LOG_SAFE(pLogger_, spdlog::level::debug, createQuery);

    sqlite3_stmt* rawPtr = nullptr;
    int result =
      sqlite3_prepare_v2(db_, createQuery.c_str(), -1, &rawPtr, nullptr);

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

  //! The internal buffer used to facilitate read/write
  //! to the database.
  std::vector<T> dataBuffer_;

  //! Tracks whether or not the DAO is initialized
  bool isInitialized_;

  //! The raw SQLite database pointer
  sqlite3* db_;

  //! The local logger
  std::shared_ptr<spdlog::logger> pLogger_;
};

}  // namespace cpp_sqlite

#endif  // DATA_ACCESS_OBJECT_HPP
