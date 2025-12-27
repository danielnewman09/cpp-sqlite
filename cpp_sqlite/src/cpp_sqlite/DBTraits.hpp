#ifndef DB_TRAITS_HPP
#define DB_TRAITS_HPP

#include <concepts>
#include <type_traits>
#include <vector>

#include "cpp_sqlite/src/cpp_sqlite/DBBaseTransferObject.hpp"

namespace cpp_sqlite
{


/*!
 * A wrapping alias for the sqlite3 prepared statement
 * that allows us to use modern C++ memory management
 * with this library.
 */
using PreparedSQLStmt =
  std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

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
struct is_vector : std::false_type
{
};

template <typename T, typename Allocator>
struct is_vector<std::vector<T, Allocator>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

template <ValidTransferObject T>
struct RepeatedFieldTransferObject;

template <ValidTransferObject T>
struct ForeignKey;

template <typename C>
concept IsRepeatedFieldTransferObject = requires(C c) {
  // 1. Check for a member named `data`
  { c.data };

  // 2. Check that the member `data` is a std::vector
  requires is_vector_v<decltype(c.data)>;

  // 3. Check the element type of the vector against the HasToString concept
  requires ValidTransferObject<typename decltype(c.data)::value_type>;
};


// --- The core template trait to extract template parameters ---
// Primary template (general case)
template <typename T>
struct GetRepeatedFieldParams
{
  static constexpr bool is_specialization = false;
};

// Partial specialization for `Foo<Bar>`
template <typename T>
struct GetRepeatedFieldParams<RepeatedFieldTransferObject<T>>
{
  static constexpr bool is_specialization = true;
  using SpecializationType = T;
};

// --- A helper alias for cleaner syntax ---
template <IsRepeatedFieldTransferObject T>
using RepeatedFieldOfType =
  typename GetRepeatedFieldParams<T>::SpecializationType;

// --- ForeignKey Type Traits ---

// Primary template for detecting ForeignKey
template <typename T>
struct is_foreign_key : std::false_type
{
};

// Specialization for ForeignKey<T>
template <ValidTransferObject T>
struct is_foreign_key<ForeignKey<T>> : std::true_type
{
};

// Concept for detecting ForeignKey types
template <typename T>
concept IsForeignKey = is_foreign_key<T>::value;

// Extract the referenced type from ForeignKey
template <typename T>
struct foreign_key_type
{
};

template <ValidTransferObject T>
struct foreign_key_type<ForeignKey<T>>
{
  using type = T;
};

// Helper alias to get the referenced type
template <IsForeignKey T>
using ForeignKeyType = typename foreign_key_type<T>::type;

// --- Basic Type Concepts ---

template <typename T>
concept isIntegral = std::integral<T>;
template <typename T>
concept floatingPoint = std::floating_point<T>;
template <typename T>
concept isString = std::is_same_v<T, std::string>;
template <typename T>
concept isBlob = std::is_same_v<T, std::vector<uint8_t>>;

/*!
 * A type supported by the database is either:
 *  - A basic integral type
 *  - A floating point type
 *  - A string
 *  - A BLOB (binary data as std::vector<uint8_t>)
 *  - A single transfer object
 *  - Or a repeated field of transfer objects
 */
template <typename T>
concept isSupportedDBType = isIntegral<T> || floatingPoint<T> || isString<T> ||
                            isBlob<T> || ValidTransferObject<T>;


}  // namespace cpp_sqlite

#endif  // DB_TRAITS_HPP