#include "EPollPoller.h"
#include "Channel.h"
#include "Logger.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <unistd.h>

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
} // namespace

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
  if (epollfd_ < 0) {
    LOG_FATAL << "EPollPoller::epoll_create1 failed: " << std::strerror(errno);
  }
}

EPollPoller::~EPollPoller() { ::close(epollfd_); }

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
  int numEvents = ::epoll_wait(epollfd_, events_.data(),
                               static_cast<int>(events_.size()), timeoutMs);
  int savedErrno = errno;
  Timestamp now = Timestamp::now();

  if (numEvents > 0) {
    fillActiveChannels(numEvents, activeChannels);
    if (static_cast<size_t>(numEvents) == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (numEvents < 0 && savedErrno != EINTR) {
    LOG_ERROR << "EPollPoller::poll failed: " << std::strerror(savedErrno);
  }

  return now;
}

void EPollPoller::updateChannel(Channel *channel) {
  const int index = channel->index();
  const int fd = channel->fd();

  if (index == kNew || index == kDeleted) {
    if (index == kNew) {
      channels_[fd] = channel;
    }
    channel->setIndex(kAdded);
    update(EPOLL_CTL_ADD, channel);
  } else {
    assert(index == kAdded);
    if (channel->isNoneEvent()) {
      update(EPOLL_CTL_DEL, channel);
      channel->setIndex(kDeleted);
    } else {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EPollPoller::removeChannel(Channel *channel) {
  const int fd = channel->fd();
  const int index = channel->index();

  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  channels_.erase(fd);

  if (index == kAdded) {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->setIndex(kNew);
}

void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList *activeChannels) const {
  for (int i = 0; i < numEvents; ++i) {
    Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
    channel->setRevents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void EPollPoller::update(int operation, Channel *channel) {
  epoll_event event {};
  event.events = channel->events();
  event.data.ptr = channel;

  if (::epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0) {
    LOG_ERROR << "EPollPoller::epoll_ctl op=" << operation
              << " fd=" << channel->fd()
              << " failed: " << std::strerror(errno);
  }
}
