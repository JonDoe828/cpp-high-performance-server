#include "Timestamp.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

TEST_CASE("Timestamp now returns valid value") {
  auto t = Timestamp::now();
  REQUIRE(!t.toString().empty());
}

TEST_CASE("Timestamp increases over time") {
  auto t1 = Timestamp::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  auto t2 = Timestamp::now();

  REQUIRE(t2.microSecondsSinceEpoch() >= t1.microSecondsSinceEpoch());
}