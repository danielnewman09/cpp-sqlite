#ifndef DATA_ACCESS_OBJECT_HPP
#define DATA_ACCESS_OBJECT_HPP

#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

#include <mutex>

#include <boost/describe.hpp>
#include <boost/describe/class.hpp>
#include <boost/mp11.hpp>
#include <boost/type_index.hpp>
#include "sqlite3.h"

#include "Logger.hpp"
#include "sqlite_db/DBDatabase.hpp"
#include "sqlite_db/DBOperations.hpp"
#include "sqlite_db/DBTraits.hpp"

namespace cpp_sqlite
{

class Database;

/*!
 * Abstract base class for all Data Access Objects
 * Provides common interface for polymorphic storage
 */
class DAOBase
{
public:
  enum class BaseSQLType : uint8_t
  {
    INT,
    FLOAT,
    TEXT,
    BLOB
  };


  virtual ~DAOBase() = default;

  virtual std::string getTableName() const = 0;

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
  DataAccessObject(Database& database,
                   std::shared_ptr<spdlog::logger> pLogger = nullptr)
    : tableName_{boost::typeindex::type_id<T>().pretty_name()},
      insertStmt_{nullptr, sqlite3_finalize},
      createStmt_{nullptr, sqlite3_finalize},
      writeBuffer_{},
      flushBuffer_{},
      idCounter_{0},
      isInitialized_{true},
      db_{database},
      pLogger_{pLogger}
  {
    isInitialized_ = executeCreateStmt();
    isInitialized_ &= prepareInsertStatement();
  }

  std::string getTableName() const
  {
    return tableName_;
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


  bool insert(T& data, uint32_t foreignKeyId)
  {
    if (!insertStmt_)
    {
      return false;  // No prepared statement available
    }

    // Increment the ID counter and update the id for the data
    // prior to calling the database insert method.
    idCounter_++;
    data.id = idCounter_;

    return db_.insert(insertStmt_, data);
  }

  bool insert(T& data)
  {
    if (!insertStmt_)
    {
      return false;  // No prepared statement available
    }

    // Increment the ID counter and update the id for the data
    // prior to calling the database insert method.
    idCounter_++;
    data.id = idCounter_;

    return db_.insert(insertStmt_, data);
  }

  /*!
   * \brief Perform an insert with the buffer data
   * Thread-safe: Swaps buffers under lock, then processes without lock
   */
  void insert() override
  {
    // Swap buffers under lock (fast operation)
    {
      std::lock_guard<std::mutex> lock(bufferMutex_);
      std::swap(writeBuffer_, flushBuffer_);
    }

    // Now process flushBuffer_ without holding the lock
    // Writers can continue adding to writeBuffer_ in parallel
    bool success = true;
    for (auto& item : flushBuffer_)
    {
      success &= insert(item);
    }

    // Clear the flush buffer after processing
    flushBuffer_.clear();
  }

  /*!
   * \brief Clear the data buffer
   */
  void clearBuffer() override
  {
    std::lock_guard<std::mutex> lock(bufferMutex_);
    writeBuffer_.clear();
    flushBuffer_.clear();
  }

  // Type-specific non-virtual methods
  /*!
   * \brief Add object to buffer for insertion (thread-safe)
   * This can be called from any thread
   */
  void addToBuffer(const T& obj)
  {
    std::lock_guard<std::mutex> lock(bufferMutex_);
    writeBuffer_.push_back(obj);
  }

private:
  // Helper function to map C++ types to SQL types
  template <isSupportedDBType FieldType>
  constexpr std::string getSQLType()
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
    std::string sql;
    sql = "CREATE TABLE IF NOT EXISTS " + tableName_ + " (";

    bool first = true;

    // Process public members
    boost::mp11::mp_for_each<boost::describe::describe_members<
      T,
      boost::describe::mod_inherited | boost::describe::mod_public>>(
      [&](auto D)
      {
        // Get member type and map to SQL type
        using memberType = std::remove_cv_t<
          std::remove_reference_t<decltype(std::declval<T>().*D.pointer)>>;


        if constexpr (IsRepeatedFieldTransferObject<memberType>)
        {
          using fieldType = RepeatedFieldOfType<memberType>;
          std::string dataName =
            boost::typeindex::type_id<fieldType>().pretty_name();
          std::string mapTable = "CREATE TABLE IF NOT EXISTS " + tableName_ +
                                 "_" + dataName + "(" + tableName_ +
                                 "_id INTEGER, " + dataName + "_id INTEGER); ";

          char* err_msg = 0;

          LOG_SAFE(
            pLogger_, spdlog::level::debug, "Create Table: {}", mapTable);

          int result =
            sqlite3_exec(&db_.getRawDB(), mapTable.c_str(), 0, 0, &err_msg);

          if (result != SQLITE_OK)
          {
            LOG_SAFE(pLogger_, spdlog::level::err, "SQL error: {}", err_msg);
          }
        }
        else
        {
          if (!first)
            sql += ", ";

          // Get member name
          sql += D.name;

          // If the field is, itself, another transfer object, we treat
          // this field in the current table as a key to the table
          // that the child object presumably exists at.
          if constexpr (ValidTransferObject<memberType>)
          {
            using idType = decltype(std::declval<memberType>().id);
            sql += "_id " + getSQLType<idType>();

            sql += ", FOREIGN KEY (" + D.name + "_id) REFERENCES " +
                   boost::typeindex::type_id<T>().pretty_name() + "(id)";
          }
          else if constexpr (isSupportedDBType<memberType>)
          {
            sql += " " + getSQLType<memberType>();

            // Add PRIMARY KEY for id field
            if (std::string(D.name) == "id")
            {
              sql += " PRIMARY KEY";
            }
          }

          first = false;
        }
      });

    sql += ");";
    return sql;
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
    int result = sqlite3_prepare_v2(
      &(db_.getRawDB()), insertQuery.c_str(), -1, &rawPtr, nullptr);

    if (result != SQLITE_OK)
    {
      LOG_SAFE(
        pLogger_,
        spdlog::level::err,
        "Could not prepare insert statement for table {}. SQLITE code: {}",
        tableName_,
        result);
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
          placeholders.push_back("?");
        }
        else if constexpr (isSupportedDBType<memberType>)
        {
          columns.push_back(std::string(D.name));
          placeholders.push_back("?");
        }
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

    int result = sqlite3_exec(&db_.getRawDB(), createQuery.c_str(), 0, 0, 0);

    if (result != SQLITE_OK)
    {
      LOG_SAFE(pLogger_,
               spdlog::level::err,
               "Could not execute query. Result code: {}",
               result);

      return false;
    }

    return true;
  }

  //! The name of the table accessed by this object.
  std::string tableName_;

  //!< The prepared statement to facilitate inserting
  //!< data into the database
  PreparedSQLStmt insertStmt_;

  //!<
  PreparedSQLStmt createStmt_;

  //! Write buffer - writers add here (protected by mutex)
  std::vector<T> writeBuffer_;

  //! Flush buffer - DB thread reads from here (no lock needed during flush)
  std::vector<T> flushBuffer_;

  //! Mutex protecting the write buffer
  std::mutex bufferMutex_;

  //! The current ID counter for inserting new data
  uint32_t idCounter_;

  //! Tracks whether or not the DAO is initialized
  bool isInitialized_;

  //! Reference to the Database object
  //! NOTE: This DAO must not outlive the Database object that created it.
  //! The Database class manages DAO lifetime through its internal storage.
  Database& db_;

  //! The local logger
  std::shared_ptr<spdlog::logger> pLogger_;
};

}  // namespace cpp_sqlite

#endif  // DATA_ACCESS_OBJECT_HPP
