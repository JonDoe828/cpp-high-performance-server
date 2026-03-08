#include "noncopyable.h"
#include <catch2/catch_test_macros.hpp>
#include <type_traits>

class TestClass : public noncopyable {};

TEST_CASE("noncopyable works") {
  TestClass obj;
  REQUIRE(true);
}

TEST_CASE("noncopyable disables copy and assignment") {
  STATIC_REQUIRE(!std::is_copy_constructible<TestClass>::value);
  STATIC_REQUIRE(!std::is_copy_assignable<TestClass>::value);
}
