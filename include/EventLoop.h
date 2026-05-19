#pragma once

#include "Poller.h"
#include "Timestamp.h"
#include "noncopyable.h"

#include <memory>

class Channel;

class EventLoop : noncopyable {
public:
  EventLoop();
  ~EventLoop();

  void loop();
  void quit();

  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  bool hasChannel(Channel *channel) const;

private:
  using ChannelList = Poller::ChannelList;

  bool looping_;
  bool quit_;
  Timestamp pollReturnTime_;
  std::unique_ptr<Poller> poller_;
  ChannelList activeChannels_;
};
