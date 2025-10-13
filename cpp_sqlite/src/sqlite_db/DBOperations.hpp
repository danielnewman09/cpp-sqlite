#ifndef DB_OPERATIONS_HPP
#define DB_OPERATIONS_HPP

#include <memory>
#include <string>

#include <boost/describe.hpp>
#include <boost/mp11.hpp>
#include "sqlite3.h"

#include "Logger.hpp"
#include "sqlite_db/DBDatabase.hpp"
#include "sqlite_db/DBTraits.hpp"

namespace cpp_sqlite
{

/*!
 * \brief Perform a generic insert operation
 *
 * \tparam T The transfer object type
 *
 * \param stmt The prepared SQL statement
 * \param data The data to insert
 * \param database Reference to the Database for nested object insertion
 * \param pLogger Optional logger for error reporting
 * \return true if successful, false otherwise
 */
template <ValidTransferObject T>
bool insert(PreparedSQLStmt& stmt,
            T& data,
            Database& database,
            std::shared_ptr<spdlog::logger> pLogger = nullptr)
{
  // Reset the statement for reuse
  sqlite3_reset(stmt.get());

  // Track parameter index (SQLite uses 1-based indexing)
  int paramIndex = 1;

  // Process public members
  boost::mp11::mp_for_each<boost::describe::describe_members<
    T,
    boost::describe::mod_inherited | boost::describe::mod_public>>(
    [&](auto D)
    {
      // Get member type and map to SQL type
      using memberType = std::remove_cv_t<
        std::remove_reference_t<decltype(std::declval<T>().*D.pointer)>>;

      // If the field is, itself, another transfer object, we will
      // (1) insert the object into its own table
      // (2) insert the ID of the created object into this table
      if constexpr (ValidTransferObject<memberType>)
      {
        // Bind the ID of the nested object
        const auto& nestedObj = data.*D.pointer;

        // Get the DAO from the database and insert nested object
        database.getDAO<memberType>().addToBuffer(nestedObj);
        database.getDAO<memberType>().insert();

        if constexpr (isIntegral<decltype(nestedObj.id)>)
        {
          sqlite3_bind_int64(
            stmt.get(), paramIndex, static_cast<sqlite3_int64>(nestedObj.id));
        }
        paramIndex++;
      }
      else
      {
        const auto& value = data.*D.pointer;

        if constexpr (isIntegral<memberType>)
        {
          sqlite3_bind_int64(
            stmt.get(), paramIndex, static_cast<sqlite3_int64>(value));
        }
        else if constexpr (floatingPoint<memberType>)
        {
          sqlite3_bind_double(
            stmt.get(), paramIndex, static_cast<double>(value));
        }
        else if constexpr (std::is_same_v<memberType, std::string>)
        {
          sqlite3_bind_text(stmt.get(),
                            paramIndex,
                            value.c_str(),
                            static_cast<int>(value.length()),
                            SQLITE_TRANSIENT);
        }
        else
        {
          // For BLOB or unknown types, bind as null for now
          sqlite3_bind_null(stmt.get(), paramIndex);
        }
        paramIndex++;
      }
    });

  // Execute the statement
  int result = sqlite3_step(stmt.get());

  if (result != SQLITE_DONE)
  {
    LOG_SAFE(
      pLogger, spdlog::level::err, "Insert failed with code: {}", result);
  }

  return result == SQLITE_DONE;
}

}  // namespace cpp_sqlite


#endif  // DB_OPERATIONS_HPP
