#include "CurrentThread.h"
#include "Thread.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

TEST_CASE("CurrentThread caches a valid Linux thread id") {
  const int first = CurrentThread::tid();
  const int second = CurrentThread::tid();

  REQUIRE(first > 0);
  REQUIRE(second == first);
}

TEST_CASE("Thread publishes worker tid and joins") {
  std::atomic<int> observedTid(0);
  std::atomic<bool> completed(false);

  Thread worker(
      [&] {
        observedTid.store(CurrentThread::tid());
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        completed.store(true);
      },
      "worker");

  worker.start();

  REQUIRE(worker.started());
  REQUIRE(worker.tid() > 0);
  REQUIRE(worker.tid() != CurrentThread::tid());

  worker.join();

  REQUIRE(worker.joined());
  REQUIRE(observedTid.load() == worker.tid());
  REQUIRE(completed.load());
  REQUIRE(worker.name() == "worker");
}

TEST_CASE("Thread assigns names and rejects invalid lifecycle calls") {
  const int createdBefore = Thread::numCreated();
  Thread worker([] {});

  REQUIRE(Thread::numCreated() == createdBefore + 1);
  REQUIRE(!worker.name().empty());
  REQUIRE_THROWS_AS(worker.join(), std::logic_error);

  worker.start();
  REQUIRE_THROWS_AS(worker.start(), std::logic_error);
  worker.join();
  REQUIRE_THROWS_AS(worker.join(), std::logic_error);
}
