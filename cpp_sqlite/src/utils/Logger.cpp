#include "Logger.hpp"

namespace cpp_sqlite
{

// Static member definitions
std::unique_ptr<Logger> Logger::instance_ = nullptr;
std::once_flag Logger::initialized_;

}  // namespace cpp_sqlite