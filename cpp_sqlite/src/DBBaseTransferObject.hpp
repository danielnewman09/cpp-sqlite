#ifndef BASE_TRANSFER_OBJECT_HPP
#define BASE_TRANSFER_OBJECT_HPP

#include <cstdint>

#include <boost/describe.hpp>
#include <boost/describe/class.hpp>

namespace cpp_sqlite
{

struct BaseTransferObject
{
  //! The unique identifier for the base
  //! Transfer object
  uint32_t id;
};

// Register the base transfer object with
// boost::describe
BOOST_DESCRIBE_STRUCT(BaseTransferObject, (), (id));

}  // namespace cpp_sqlite

#endif  // BASE_TRANSFER_OBJECT_HPP