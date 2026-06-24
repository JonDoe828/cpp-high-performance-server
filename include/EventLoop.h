#pragma once

#include "Poller.h"
#include "Timestamp.h"
#include "noncopyable.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <sys/types.h>
#include <vector>

class Channel;

class EventLoop : noncopyable {
public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();

  void loop();
  void quit();

  void runInLoop(Functor cb);
  void queueInLoop(Functor cb);

  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  bool hasChannel(Channel *channel) const;

  bool isInLoopThread() const;
  Timestamp pollReturnTime() const;

private:
  using ChannelList = Poller::ChannelList;

  void wakeup();
  void handleRead();
  void doPendingFunctors();

  std::atomic_bool looping_;
  std::atomic_bool quit_;
  std::atomic_bool callingPendingFunctors_;
  const pid_t threadId_;
  Timestamp pollReturnTime_;
  std::unique_ptr<Poller> poller_;
  int wakeupFd_;
  std::unique_ptr<Channel> wakeupChannel_;
  ChannelList activeChannels_;
  std::mutex mutex_;
  std::vector<Functor> pendingFunctors_;
};
