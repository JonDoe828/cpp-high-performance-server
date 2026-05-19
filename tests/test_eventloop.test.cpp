#include "Channel.h"
#include "EventLoop.h"
#include "Timestamp.h"

#include <catch2/catch_test_macros.hpp>

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
