#ifndef DB_TRAITS_HPP
#define DB_TRAITS_HPP

#include <concepts>
#include <type_traits>

#include "sqlite_db/DBBaseTransferObject.hpp"

namespace cpp_sqlite
{

// Primary concept: Must derive from BaseTransferObject
template <typename T>
concept TransferObject = std::derived_from<T, BaseTransferObject>;

// Extended concept: Must be a transfer object with default constructor
template <typename T>
concept DefaultConstructibleTransferObject =
  TransferObject<T> && std::default_initializable<T>;

// Extended concept: Must be a transfer object that's copyable
template <typename T>
concept CopyableTransferObject = TransferObject<T> && std::copyable<T>;

// Extended concept: Must be a transfer object that's movable
template <typename T>
concept MovableTransferObject = TransferObject<T> && std::movable<T>;

// Comprehensive concept combining common requirements
template <typename T>
concept ValidTransferObject =
  TransferObject<T> && DefaultConstructibleTransferObject<T>;

template <typename T>
concept isIntegral = std::integral<T>;
template <typename T>
concept floatingPoint = std::floating_point<T>;
template <typename T>
concept isString = std::is_same_v<T, std::string>;

/*!
 * A type supported by the database is either:
 *  - A basic integral type
 *  - A floating point type
 *  - A string
 *  - Or a transfer object
 */
template <typename T>
concept isSupportedDBType =
  isIntegral<T> || floatingPoint<T> || isString<T> || ValidTransferObject<T>;


}  // namespace cpp_sqlite

#endif  // DB_TRAITS_HPP