#include "Thread.h"
#include "CurrentThread.h"

#include <future>
#include <stdexcept>
#include <utility>

std::atomic<int> Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, std::string name)
    : started_(false),
      joined_(false),
      tid_(0),
      func_(std::move(func)),
      name_(std::move(name)) {
  setDefaultName();
}

Thread::~Thread() {
  if (started_ && !joined_ && thread_ && thread_->joinable()) {
    thread_->detach();
  }
}

void Thread::start() {
  if (started_) {
    throw std::logic_error("Thread::start called more than once");
  }

  std::shared_ptr<std::promise<pid_t>> tidPromise(
      new std::promise<pid_t>());
  std::future<pid_t> tidFuture = tidPromise->get_future();
  ThreadFunc func = func_;

  thread_.reset(new std::thread([tidPromise, func] {
    tidPromise->set_value(static_cast<pid_t>(CurrentThread::tid()));
    func();
  }));

  started_ = true;
  tid_ = tidFuture.get();
}

void Thread::join() {
  if (!started_) {
    throw std::logic_error("Thread::join called before start");
  }
  if (joined_) {
    throw std::logic_error("Thread::join called more than once");
  }

  joined_ = true;
  thread_->join();
}

bool Thread::started() const { return started_; }

bool Thread::joined() const { return joined_; }

pid_t Thread::tid() const { return tid_; }

const std::string &Thread::name() const { return name_; }

int Thread::numCreated() { return numCreated_.load(); }

void Thread::setDefaultName() {
  const int number = ++numCreated_;
  if (name_.empty()) {
    name_ = "Thread" + std::to_string(number);
  }
}
