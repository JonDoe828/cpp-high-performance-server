#include "Logger.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Logger basic logging does not throw") {
  REQUIRE_NOTHROW(Logger::info("hello"));
  REQUIRE_NOTHROW(Logger::error("error"));
}

TEST_CASE("Logger handles empty message") {
  REQUIRE_NOTHROW(Logger::info(""));
  REQUIRE_NOTHROW(Logger::error(""));
}
