#include "../include/Timestamp.h"

#include <ctime>
#include <iomanip>
#include <sstream>

Timestamp::Timestamp() = default;

Timestamp Timestamp::now() {
  Timestamp t;
  t.time_ = std::chrono::system_clock::now();
  return t;
}

int64_t Timestamp::microSecondsSinceEpoch() const {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             time_.time_since_epoch())
      .count();
}

std::string Timestamp::toString() const {
  const auto micros = microSecondsSinceEpoch();
  const std::time_t seconds = static_cast<std::time_t>(micros / 1000000);
  const int64_t micros_part = micros % 1000000;

  std::tm tm_time {};
  localtime_r(&seconds, &tm_time);

  std::ostringstream oss;
  oss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S") << '.'
      << std::setw(6) << std::setfill('0') << micros_part;
  return oss.str();
}
