#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

#include <cassert>
#include <stdexcept>
#include <utility>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, std::string name)
    : baseLoop_(baseLoop),
      name_(std::move(name)),
      started_(false),
      numThreads_(0),
      next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() = default;

void EventLoopThreadPool::setThreadNum(int numThreads) {
  if (started_) {
    throw std::logic_error("EventLoopThreadPool::setThreadNum after start");
  }
  if (numThreads < 0) {
    throw std::invalid_argument("EventLoopThreadPool thread count is negative");
  }
  numThreads_ = numThreads;
}

void EventLoopThreadPool::start() {
  if (started_) {
    throw std::logic_error("EventLoopThreadPool::start called more than once");
  }

  started_ = true;

  for (int i = 0; i < numThreads_; ++i) {
    std::string threadName = name_ + "-" + std::to_string(i);
    std::unique_ptr<EventLoopThread> thread(new EventLoopThread(threadName));
    EventLoop *loop = thread->startLoop();
    threads_.push_back(std::move(thread));
    loops_.push_back(loop);
  }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
  assert(started_);

  EventLoop *loop = baseLoop_;
  if (!loops_.empty()) {
    loop = loops_[next_];
    ++next_;
    if (static_cast<size_t>(next_) >= loops_.size()) {
      next_ = 0;
    }
  }
  return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() const {
  if (loops_.empty()) {
    return std::vector<EventLoop *>(1, baseLoop_);
  }
  return loops_;
}

bool EventLoopThreadPool::started() const { return started_; }

const std::string &EventLoopThreadPool::name() const { return name_; }
