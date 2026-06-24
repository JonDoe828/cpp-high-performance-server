#pragma once

#include "noncopyable.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <sys/types.h>
#include <thread>

class Thread : noncopyable {
public:
  using ThreadFunc = std::function<void()>;

  explicit Thread(ThreadFunc func, std::string name = std::string());
  ~Thread();

  void start();
  void join();

  bool started() const;
  bool joined() const;
  pid_t tid() const;
  const std::string &name() const;

  static int numCreated();

private:
  void setDefaultName();

  bool started_;
  bool joined_;
  std::unique_ptr<std::thread> thread_;
  pid_t tid_;
  ThreadFunc func_;
  std::string name_;

  static std::atomic<int> numCreated_;
};
