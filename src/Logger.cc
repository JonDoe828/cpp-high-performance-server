#include "Logger.h"
#include "Timestamp.h"

#include <cstdlib>
#include <iostream>
#include <mutex>
#include <utility>

namespace {
std::mutex g_outputMutex;

void defaultOutput(const char *msg, int len) {
  const std::string line(msg, len);
  const bool isError = line.find("[ERROR]") != std::string::npos ||
                       line.find("[FATAL]") != std::string::npos;
  std::ostream &out = isError ? std::cerr : std::cout;
  out.write(msg, len);
  out.flush();
}

Logger::OutputFunc &outputFunc() {
  static Logger::OutputFunc output = defaultOutput;
  return output;
}

const char *levelName(Logger::Level level) {
  switch (level) {
  case Logger::Level::DEBUG:
    return "DEBUG";
  case Logger::Level::INFO:
    return "INFO";
  case Logger::Level::WARN:
    return "WARN";
  case Logger::Level::ERROR:
    return "ERROR";
  case Logger::Level::FATAL:
    return "FATAL";
  }
  return "INFO";
}
} // namespace

Logger::Logger(const char *file, int line, Level level)
    : file_(file), line_(line), level_(level) {}

Logger::~Logger() {
  log(level_, file_, line_, stream_.str());
  if (level_ == Level::FATAL) {
    std::abort();
  }
}

std::ostringstream &Logger::stream() { return stream_; }

void Logger::setOutput(OutputFunc output) {
  std::lock_guard<std::mutex> lock(g_outputMutex);
  outputFunc() = output ? std::move(output) : OutputFunc(defaultOutput);
}

void Logger::info(const std::string &msg) {
  log(Level::INFO, __FILE__, __LINE__, msg);
}

void Logger::warn(const std::string &msg) {
  log(Level::WARN, __FILE__, __LINE__, msg);
}

void Logger::error(const std::string &msg) {
  log(Level::ERROR, __FILE__, __LINE__, msg);
}

void Logger::log(Level level, const char *file, int line, const std::string &msg) {
  std::ostringstream output;
  output << Timestamp::now().toFormattedString() << " [" << levelName(level)
         << "] " << file << ':' << line << " - " << msg << '\n';

  const std::string lineText = output.str();
  std::lock_guard<std::mutex> lock(g_outputMutex);
  outputFunc()(lineText.data(), static_cast<int>(lineText.size()));
}
