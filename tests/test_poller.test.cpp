#include "Channel.h"
#include "EPollPoller.h"

#include <catch2/catch_test_macros.hpp>

#include <unistd.h>

TEST_CASE("EPollPoller returns active readable channel") {
  int fds[2];
  REQUIRE(::pipe(fds) == 0);

  EPollPoller poller(nullptr);
  Channel channel(nullptr, fds[0]);
  channel.enableReading();
  poller.updateChannel(&channel);

  char byte = 'x';
  REQUIRE(::write(fds[1], &byte, 1) == 1);

  Poller::ChannelList activeChannels;
  Timestamp receiveTime = poller.poll(1000, &activeChannels);

  REQUIRE(receiveTime.valid());
  REQUIRE(activeChannels.size() == 1);
  REQUIRE(activeChannels[0] == &channel);

  channel.disableAll();
  poller.updateChannel(&channel);
  poller.removeChannel(&channel);

  ::close(fds[0]);
  ::close(fds[1]);
}
