
#ifndef DB_DATABASE_HPP
#define DB_DATABASE_HPP

#include <any>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <boost/describe.hpp>
#include <boost/mp11.hpp>
#include <boost/unordered_map.hpp>
#include "sqlite3.h"

#include "Logger.hpp"
#include "sqlite_db/DBBaseTransferObject.hpp"
#include "sqlite_db/DBTraits.hpp"

namespace cpp_sqlite
{

class DAOBase;

template <ValidTransferObject T>
class DataAccessObject;


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
   * \brief Get or create a DAO for the specified type
   */
  template <ValidTransferObject T>
  DataAccessObject<T>& getDAO()
  {
    auto typeIdx = std::type_index(typeid(T));
    auto it = daos_.find(typeIdx);

    if (it == daos_.end())
    {
      auto dao = std::make_unique<DataAccessObject<T>>(*this, pLogger_);
      auto& daoRef = *dao;
      daos_.emplace(typeIdx, std::move(dao));
      return daoRef;
    }

    // Safe static_cast - we know the type from the map key
    return static_cast<DataAccessObject<T>&>(*it->second);
  }

  /*!
   * \brief Cascading insert that handles nested transfer objects
   */
  template <ValidTransferObject T>
  void cascadingInsert(const T& obj)
  {
    cascadingInsertImpl(obj);
  }

  /*!
   * \brief Perform a generic insert operation
   */
  template <ValidTransferObject T>
  bool insert(PreparedSQLStmt& stmt, T& data)
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
          auto& nestedObj = data.*D.pointer;

          getDAO<memberType>().insert(nestedObj);

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
        pLogger_, spdlog::level::err, "Insert failed with code: {}", result);
    }

    return result == SQLITE_DONE;
  }

  /*!
   * \brief Get raw SQLite database pointer for direct access
   * \return Raw sqlite3* pointer
   */
  sqlite3& getRawDB();

private:
  /*!
   * \brief Implementation of cascading insert
   */
  template <ValidTransferObject T>
  void cascadingInsertImpl(const T& obj) const
  {
    // Handle nested objects first
    boost::mp11::mp_for_each<boost::describe::describe_members<
      T,
      boost::describe::mod_inherited | boost::describe::mod_public>>(
      [&](auto D)
      {
        using MemberType =
          std::remove_cv_t<std::remove_reference_t<decltype(obj.*D.pointer)>>;

        if constexpr (ValidTransferObject<MemberType>)
        {
          cascadingInsertImpl(obj.*D.pointer);
        }
      });

    // Insert current object
    auto& dao = getDAO<T>();
    dao.clearBuffer();
    dao.addToBuffer(obj);
    dao.insert();
  }

  //!< The unique pointer storing the SQLite database
  //!< object
  std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db_;

  //! The pointer to the spdlog for this object.
  std::shared_ptr<spdlog::logger> pLogger_;

  //! DAO storage using boost::unordered_map for better performance
  boost::unordered_map<std::type_index, std::unique_ptr<DAOBase>> daos_;
};


}  // namespace cpp_sqlite

#endif  // DB_DATABASE_HPP