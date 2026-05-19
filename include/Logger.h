#pragma once

#include <functional>
#include <sstream>
#include <string>

class Logger {
public:
  enum class Level {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
  };

  using OutputFunc = std::function<void(const char *, int)>;

  Logger(const char *file, int line, Level level);
  ~Logger();

  std::ostringstream &stream();

  static void setOutput(OutputFunc output);
  static void info(const std::string &msg);
  static void warn(const std::string &msg);
  static void error(const std::string &msg);

private:
  static void log(Level level, const char *file, int line,
                  const std::string &msg);

  const char *file_;
  int line_;
  Level level_;
  std::ostringstream stream_;
};

#define LOG_DEBUG Logger(__FILE__, __LINE__, Logger::Level::DEBUG).stream()
#define LOG_INFO Logger(__FILE__, __LINE__, Logger::Level::INFO).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::Level::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::Level::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::Level::FATAL).stream()
