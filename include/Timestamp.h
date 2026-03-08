#pragma once

#include <cstdint>
#include <chrono>
#include <string>

class Timestamp {
public:
  Timestamp();

  static Timestamp now();
  int64_t microSecondsSinceEpoch() const;

  std::string toString() const;

private:
  std::chrono::system_clock::time_point time_;
};
