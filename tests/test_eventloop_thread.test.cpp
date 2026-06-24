#include "CurrentThread.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <future>
#include <stdexcept>
#include <vector>

namespace {
bool ready(std::future<int> &future) {
  return future.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
}
} // namespace

TEST_CASE("EventLoopThread starts loop in worker thread") {
  std::promise<int> initTidPromise;
  std::future<int> initTidFuture = initTidPromise.get_future();

  EventLoopThread loopThread(
      "worker-loop",
      [&](EventLoop *) { initTidPromise.set_value(CurrentThread::tid()); });

  EventLoop *loop = loopThread.startLoop();
  REQUIRE(loop != nullptr);
  REQUIRE(!loop->isInLoopThread());

  REQUIRE(ready(initTidFuture));
  const int mainTid = CurrentThread::tid();
  REQUIRE(initTidFuture.get() != mainTid);

  std::promise<int> callbackTidPromise;
  std::future<int> callbackTidFuture = callbackTidPromise.get_future();
  loop->queueInLoop(
      [&] { callbackTidPromise.set_value(CurrentThread::tid()); });

  REQUIRE(ready(callbackTidFuture));
  REQUIRE(callbackTidFuture.get() != mainTid);
}

TEST_CASE("EventLoopThread rejects repeated start") {
  EventLoopThread loopThread("single-start");
  EventLoop *loop = loopThread.startLoop();

  REQUIRE(loop != nullptr);
  REQUIRE_THROWS_AS(loopThread.startLoop(), std::logic_error);
}

TEST_CASE("EventLoopThreadPool returns base loop when thread count is zero") {
  EventLoop baseLoop;
  EventLoopThreadPool pool(&baseLoop, "zero");

  REQUIRE(!pool.started());
  pool.start();

  REQUIRE(pool.started());
  REQUIRE(pool.getNextLoop() == &baseLoop);

  std::vector<EventLoop *> loops = pool.getAllLoops();
  REQUIRE(loops.size() == 1);
  REQUIRE(loops[0] == &baseLoop);
}

TEST_CASE("EventLoopThreadPool starts workers and rotates loops") {
  EventLoop baseLoop;
  EventLoopThreadPool pool(&baseLoop, "io");
  pool.setThreadNum(2);
  pool.start();

  std::vector<EventLoop *> loops = pool.getAllLoops();
  REQUIRE(loops.size() == 2);
  REQUIRE(loops[0] != &baseLoop);
  REQUIRE(loops[1] != &baseLoop);
  REQUIRE(loops[0] != loops[1]);

  REQUIRE(pool.getNextLoop() == loops[0]);
  REQUIRE(pool.getNextLoop() == loops[1]);
  REQUIRE(pool.getNextLoop() == loops[0]);

  const int mainTid = CurrentThread::tid();
  std::promise<int> firstTidPromise;
  std::promise<int> secondTidPromise;
  std::future<int> firstTidFuture = firstTidPromise.get_future();
  std::future<int> secondTidFuture = secondTidPromise.get_future();

  loops[0]->queueInLoop([&] { firstTidPromise.set_value(CurrentThread::tid()); });
  loops[1]->queueInLoop(
      [&] { secondTidPromise.set_value(CurrentThread::tid()); });

  REQUIRE(ready(firstTidFuture));
  REQUIRE(ready(secondTidFuture));
  REQUIRE(firstTidFuture.get() != mainTid);
  REQUIRE(secondTidFuture.get() != mainTid);
}

TEST_CASE("EventLoopThreadPool rejects invalid lifecycle calls") {
  EventLoop baseLoop;
  EventLoopThreadPool pool(&baseLoop, "invalid");

  REQUIRE_THROWS_AS(pool.setThreadNum(-1), std::invalid_argument);
  pool.start();
  REQUIRE_THROWS_AS(pool.setThreadNum(1), std::logic_error);
  REQUIRE_THROWS_AS(pool.start(), std::logic_error);
}
