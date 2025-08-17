#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <format>
#include <memory>
#include <mutex>
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

  // Default move constructor and assignment operator
  Logger(Logger&& logger) = default;
  Logger& operator=(Logger&& rhs) = default;


  // Get the singleton instance
  static Logger& getInstance()
  {
    std::call_once(initialized_, &Logger::initialize);
    return *instance_;
  }

  // Get the shared logger pointer
  std::shared_ptr<spdlog::logger> getLogger() const
  {
    return logger_;
  }

  // Configure logger with file and console output
  void configure(const std::string& loggerName = "cpp_sqlite",
                 const std::string& logFile = "cpp_sqlite.log",
                 spdlog::level::level_enum level = spdlog::level::info)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    try
    {
      // Create console sink
      auto console_sink =
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      console_sink->set_level(level);
      console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");

      // Create file sink
      auto file_sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);
      file_sink->set_level(level);
      file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [thread %t] %v");

      // Create logger with both sinks
      std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};
      logger_ = std::make_shared<spdlog::logger>(
        loggerName, sinks.begin(), sinks.end());

      logger_->set_level(level);
      logger_->flush_on(spdlog::level::warn);

      // Register with spdlog
      spdlog::register_logger(logger_);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
      throw std::runtime_error("Logger configuration failed: " +
                               std::string(ex.what()));
    }
  }

  // Set log level
  void setLevel(spdlog::level::level_enum level)
  {
    if (logger_)
    {
      logger_->set_level(level);
    }
  }

  // Convenience methods
  template <typename... Args>
  void info(std::format_string<Args...> fmt, Args&&... args)
  {
    if (logger_)
    {
      logger_->info(fmt, std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  void warn(std::format_string<Args...> fmt, Args&&... args)
  {
    if (logger_)
    {
      logger_->warn(fmt, std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  void error(std::format_string<Args...> fmt, Args&&... args)
  {
    if (logger_)
    {
      logger_->error(fmt, std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  void debug(std::format_string<Args...> fmt, Args&&... args)
  {
    if (logger_)
    {
      logger_->debug(fmt, std::forward<Args>(args)...);
    }
  }

private:
  Logger() = default;
  ~Logger() = default;

  static void initialize()
  {
    instance_ = std::unique_ptr<Logger>(new Logger());
    // Configure with default settings
    instance_->configure();
  }

  static std::unique_ptr<Logger> instance_;
  static std::once_flag initialized_;

  std::shared_ptr<spdlog::logger> logger_;
  mutable std::mutex mutex_;
};

// Convenience macros
#define LOG_INFO(...) cpp_sqlite::Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARN(...) cpp_sqlite::Logger::getInstance().warn(__VA_ARGS__)
#define LOG_ERROR(...) cpp_sqlite::Logger::getInstance().error(__VA_ARGS__)
#define LOG_DEBUG(...) cpp_sqlite::Logger::getInstance().debug(__VA_ARGS__)

#define GET_LOGGER() cpp_sqlite::Logger::getInstance().getLogger()

}  // namespace cpp_sqlite

#endif  // LOGGER_HPP