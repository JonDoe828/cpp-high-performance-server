#pragma once

#include "Poller.h"

#include <sys/epoll.h>

#include <vector>

class EPollPoller : public Poller {
public:
  explicit EPollPoller(EventLoop *loop);
  ~EPollPoller() override;

  Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
  void updateChannel(Channel *channel) override;
  void removeChannel(Channel *channel) override;

private:
  using EventList = std::vector<epoll_event>;

  void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
  void update(int operation, Channel *channel);

  static const int kInitEventListSize = 16;

  int epollfd_;
  EventList events_;
};
