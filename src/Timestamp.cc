#include "Timestamp.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

Timestamp Timestamp::invalid() { return Timestamp(); }

Timestamp Timestamp::now() {
  const auto now = std::chrono::system_clock::now();
  const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
                          now.time_since_epoch())
                          .count();
  return Timestamp(micros);
}

bool Timestamp::valid() const { return microSecondsSinceEpoch_ > 0; }

int64_t Timestamp::microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

std::time_t Timestamp::secondsSinceEpoch() const {
  return static_cast<std::time_t>(microSecondsSinceEpoch_ /
                                  kMicroSecondsPerSecond);
}

std::string Timestamp::toString() const {
  std::ostringstream oss;
  oss << secondsSinceEpoch() << '.'
      << std::setw(6) << std::setfill('0')
      << microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
  return oss.str();
}

std::string Timestamp::toFormattedString(bool showMicroseconds) const {
  const std::time_t seconds = secondsSinceEpoch();
  const int64_t microsPart =
      microSecondsSinceEpoch_ % kMicroSecondsPerSecond;

  std::tm tm_time {};
  localtime_r(&seconds, &tm_time);

  std::ostringstream oss;
  oss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
  if (showMicroseconds) {
    oss << '.' << std::setw(6) << std::setfill('0') << microsPart;
  }
  return oss.str();
}
