#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <format>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace cpp_sqlite
{

class Logger
{
public:
  // Delete copy constructor and assignment operator
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  // Delete move constructor and assignment operator
  Logger(Logger&& logger) = delete;
  Logger& operator=(Logger&& rhs) = delete;

  // Get the singleton instance
  static Logger& getInstance()
  {
    static Logger instance;
    return instance;
  }

  // Configure logger with file and console output
  void configure(const std::string& loggerName = "cpp_sqlite",
                 const std::string& logFile = "cpp_sqlite.log",
                 spdlog::level::level_enum level = spdlog::level::info);

  // Set log level
  void setLevel(spdlog::level::level_enum level);

  // Check if logger is configured
  bool isConfigured() const;

  // Get reference to the logger
  std::shared_ptr<spdlog::logger> getLogger() const;

private:
  Logger();
  ~Logger() = default;

  std::shared_ptr<spdlog::logger> logger_;
};


}  // namespace cpp_sqlite

#endif  // LOGGER_HPP