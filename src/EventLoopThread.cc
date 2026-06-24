#include "EventLoopThread.h"
#include "EventLoop.h"

#include <stdexcept>
#include <utility>

EventLoopThread::EventLoopThread(std::string name, ThreadInitCallback cb)
    : loop_(nullptr),
      exiting_(false),
      thread_([this] { threadFunc(); }, std::move(name)),
      callback_(std::move(cb)) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;

  EventLoop *loop = nullptr;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    loop = loop_;
  }

  if (loop) {
    loop->quit();
  }

  if (thread_.started() && !thread_.joined()) {
    thread_.join();
  }
}

EventLoop *EventLoopThread::startLoop() {
  if (thread_.started()) {
    throw std::logic_error("EventLoopThread::startLoop called more than once");
  }

  thread_.start();

  EventLoop *loop = nullptr;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] { return loop_ != nullptr; });
    loop = loop_;
  }

  return loop;
}

void EventLoopThread::threadFunc() {
  EventLoop loop;

  if (callback_) {
    callback_(&loop);
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = &loop;
    cond_.notify_one();
  }

  loop.loop();

  {
    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr;
  }
}
