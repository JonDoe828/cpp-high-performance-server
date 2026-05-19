#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"

namespace {
const int kPollTimeMs = 10000;
} // namespace

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      poller_(Poller::newDefaultPoller(this)) {}

EventLoop::~EventLoop() = default;

void EventLoop::loop() {
  if (looping_) {
    LOG_ERROR << "EventLoop::loop called while already looping";
    return;
  }

  looping_ = true;
  quit_ = false;

  while (!quit_) {
    activeChannels_.clear();
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

    for (Channel *channel : activeChannels_) {
      channel->handleEvent(pollReturnTime_);
    }
  }

  looping_ = false;
}

void EventLoop::quit() { quit_ = true; }

void EventLoop::updateChannel(Channel *channel) {
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) const {
  return poller_->hasChannel(channel);
}
