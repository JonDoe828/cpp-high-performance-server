#pragma once

#include <string>
#include <cstdint>
#include <ctime>

class Timestamp {
public:
  Timestamp();
  explicit Timestamp(int64_t microSecondsSinceEpoch);

  static Timestamp invalid();
  static Timestamp now();

  bool valid() const;
  int64_t microSecondsSinceEpoch() const;
  std::time_t secondsSinceEpoch() const;

  std::string toString() const;
  std::string toFormattedString(bool showMicroseconds = true) const;

  static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
  int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline bool operator>(Timestamp lhs, Timestamp rhs) { return rhs < lhs; }

inline bool operator<=(Timestamp lhs, Timestamp rhs) { return !(rhs < lhs); }

inline bool operator>=(Timestamp lhs, Timestamp rhs) { return !(lhs < rhs); }

inline double timeDifference(Timestamp high, Timestamp low) {
  return static_cast<double>(high.microSecondsSinceEpoch() -
                             low.microSecondsSinceEpoch()) /
         Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds) {
  const int64_t delta =
      static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
  return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}
