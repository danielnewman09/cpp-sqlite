
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

#include <boost/type_index.hpp>
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
        else if constexpr (IsRepeatedFieldTransferObject<memberType>)
        {
          auto& repeatedFieldObj = data.*D.pointer;
          using fieldType = RepeatedFieldOfType<memberType>;

          auto memberName = getDAO<T>().getTableName();
          const std::string dataName =
            boost::typeindex::type_id<fieldType>().pretty_name();
          std::string mapTable = "INSERT INTO " + memberName + "_" + dataName +
                                 "(" + memberName + "_id, " + dataName +
                                 "_id) VALUES (?, ?);";

          sqlite3_stmt* rawPtr = nullptr;
          int result = sqlite3_prepare_v2(
            db_.get(), mapTable.c_str(), -1, &rawPtr, nullptr);


          for (auto& repeatedFieldData : repeatedFieldObj.data)
          {
            getDAO<fieldType>().insert(repeatedFieldData);


            LOG_SAFE(pLogger_,
                     spdlog::level::debug,
                     "Binding data ID: {}, and fieldID: {}",
                     data.id,
                     repeatedFieldData.id);

            sqlite3_bind_int64(rawPtr, 1, static_cast<sqlite3_int64>(data.id));
            sqlite3_bind_int64(
              rawPtr, 2, static_cast<sqlite3_int64>(repeatedFieldData.id));

            // Execute the statement
            int result = sqlite3_step(rawPtr);

            if (result != SQLITE_DONE)
            {
              LOG_SAFE(pLogger_,
                       spdlog::level::err,
                       "Insert failed with code: {}",
                       result);
            }

            // Reset the statement for reuse
            sqlite3_reset(rawPtr);
          }
          sqlite3_finalize(rawPtr);
          // do nothing
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
        }
        paramIndex++;
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