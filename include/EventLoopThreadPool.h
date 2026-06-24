#pragma once

#include "noncopyable.h"

#include <memory>
#include <string>
#include <vector>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable {
public:
  EventLoopThreadPool(EventLoop *baseLoop, std::string name);
  ~EventLoopThreadPool();

  void setThreadNum(int numThreads);
  void start();

  EventLoop *getNextLoop();
  std::vector<EventLoop *> getAllLoops() const;

  bool started() const;
  const std::string &name() const;

private:
  EventLoop *baseLoop_;
  std::string name_;
  bool started_;
  int numThreads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop *> loops_;
};
