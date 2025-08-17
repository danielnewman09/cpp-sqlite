#ifndef DATA_ACCESS_OBJECT_HPP
#define DATA_ACCESS_OBJECT_HPP

#include <vector>

#include "cpp_sqlite/src/"
#include "cpp_sqlite/src/DBTraits.hpp"

namespace cpp_sqlite
{

template <ValidTransferObject T>
class DataAccessObject
{
public:
private:
  //! The internal buffer used to facilitate read/write
  //! to the database.
  std::vector<T> dataBuffer_;
};

}  // namespace cpp_sqlite

#endif  // DATA_ACCESS_OBJECT_HPP
