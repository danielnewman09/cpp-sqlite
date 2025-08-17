#ifndef DB_TRAITS_HPP
#define DB_TRAITS_HPP

#include <concepts>
#include <type_traits>

#include "DBBaseTransferObject.hpp"

namespace cpp_sqlite
{

// Primary concept: Must derive from BaseTransferObject
template <typename T>
concept TransferObject = std::derived_from<T, BaseTransferObject>;

// Extended concept: Must be a transfer object with default constructor
template <typename T>
concept DefaultConstructibleTransferObject = TransferObject<T> && std::default_initializable<T>;

// Extended concept: Must be a transfer object that's copyable
template <typename T>
concept CopyableTransferObject = TransferObject<T> && std::copyable<T>;

// Extended concept: Must be a transfer object that's movable
template <typename T>
concept MovableTransferObject = TransferObject<T> && std::movable<T>;

// Comprehensive concept combining common requirements
template <typename T>
concept ValidTransferObject = TransferObject<T> && DefaultConstructibleTransferObject<T> &&
                              CopyableTransferObject<T> && MovableTransferObject<T>;

}  // namespace cpp_sqlite

#endif  // DB_TRAITS_HPP