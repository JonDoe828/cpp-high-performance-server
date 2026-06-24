#include "Channel.h"
#include "CurrentThread.h"
#include "EventLoop.h"
#include "Thread.h"
#include "Timestamp.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <unistd.h>

TEST_CASE("EventLoop dispatches readable channel callback") {
  int fds[2];
  REQUIRE(::pipe(fds) == 0);

  EventLoop loop;
  Channel channel(&loop, fds[0]);
  bool called = false;

  channel.setReadCallback([&](Timestamp receiveTime) {
    char byte = 0;
    REQUIRE(::read(fds[0], &byte, 1) == 1);
    REQUIRE(byte == 'x');
    REQUIRE(receiveTime.valid());

    called = true;
    channel.disableAll();
    loop.quit();
  });

  channel.enableReading();

  char byte = 'x';
  REQUIRE(::write(fds[1], &byte, 1) == 1);

  loop.loop();

  REQUIRE(called);
  channel.remove();

  ::close(fds[0]);
  ::close(fds[1]);
}

TEST_CASE("EventLoop runInLoop executes immediately in owner thread") {
  EventLoop loop;
  bool called = false;
  int callbackTid = 0;

  loop.runInLoop([&] {
    called = true;
    callbackTid = CurrentThread::tid();
  });

  REQUIRE(called);
  REQUIRE(callbackTid == CurrentThread::tid());
}

TEST_CASE("EventLoop queueInLoop can run after loop starts") {
  EventLoop loop;
  bool called = false;

  loop.queueInLoop([&] {
    called = true;
    loop.quit();
  });

  loop.loop();

  REQUIRE(called);
}

TEST_CASE("EventLoop queueInLoop wakes loop from another thread") {
  EventLoop loop;
  const int ownerTid = CurrentThread::tid();
  std::atomic<bool> called(false);
  std::atomic<int> callbackTid(0);

  Thread worker(
      [&] {
        loop.queueInLoop([&] {
          callbackTid.store(CurrentThread::tid());
          called.store(true);
          loop.quit();
        });
      },
      "eventloop-queue");

  worker.start();
  loop.loop();
  worker.join();

  REQUIRE(called.load());
  REQUIRE(callbackTid.load() == ownerTid);
}
