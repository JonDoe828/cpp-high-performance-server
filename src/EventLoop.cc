#include "EventLoop.h"
#include "Channel.h"
#include "CurrentThread.h"
#include "Logger.h"

#include <cerrno>
#include <cstring>
#include <sys/eventfd.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {
const int kPollTimeMs = 10000;

thread_local EventLoop *t_loopInThisThread = nullptr;

int createEventfd() {
  int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (eventfd < 0) {
    LOG_FATAL << "EventLoop::eventfd failed: " << std::strerror(errno);
  }
  return eventfd;
}
} // namespace

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)) {
  if (t_loopInThisThread) {
    LOG_FATAL << "another EventLoop already exists in this thread";
  }
  t_loopInThisThread = this;

  wakeupChannel_->setReadCallback([this](Timestamp) { handleRead(); });
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  ::close(wakeupFd_);
  t_loopInThisThread = nullptr;
}

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

    doPendingFunctors();
  }

  looping_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor cb) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingFunctors_.push_back(std::move(cb));
  }

  if (!isInLoopThread() || callingPendingFunctors_ || !looping_) {
    wakeup();
  }
}

void EventLoop::updateChannel(Channel *channel) {
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) const {
  return poller_->hasChannel(channel);
}

bool EventLoop::isInLoopThread() const {
  return threadId_ == CurrentThread::tid();
}

Timestamp EventLoop::pollReturnTime() const { return pollReturnTime_; }

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    LOG_ERROR << "EventLoop::wakeup wrote " << n
              << " bytes instead of " << sizeof(one)
              << ": " << std::strerror(errno);
  }
}

void EventLoop::handleRead() {
  uint64_t one = 0;
  ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one) && errno != EAGAIN && errno != EWOULDBLOCK) {
    LOG_ERROR << "EventLoop::handleRead read " << n
              << " bytes instead of " << sizeof(one)
              << ": " << std::strerror(errno);
  }
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (Functor &functor : functors) {
    functor();
  }

  callingPendingFunctors_ = false;
}
