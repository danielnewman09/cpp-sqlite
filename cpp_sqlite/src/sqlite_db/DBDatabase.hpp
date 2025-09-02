
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
#include "sqlite_db/DBDataAccessObject.hpp"
#include "sqlite_db/DBTraits.hpp"

namespace cpp_sqlite
{

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
      auto dao = std::make_unique<DataAccessObject<T>>(getRawDB(), pLogger_);
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