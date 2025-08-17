#ifndef REPEATED_FIELD_TRANSFER_OBJECT_HPP
#define REPEATED_FIELD_TRANSFER_OBJECT_HPP

#include <vector>

#include <boost/describe.hpp>
#include <boost/describe/class.hpp>
#include "DBTraits.hpp"

namespace cpp_sqlite
{

template <TransferObject T>
struct RepeatedFieldTransferObject
{
  //!< The underlying vector of transfer objects
  //!< contained by this object.
  std::vector<T> data;
};

}  // namespace cpp_sqlite

#endif  // REPEATED_FIELD_TRANSFER_OBJECT_HPP