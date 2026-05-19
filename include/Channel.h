#pragma once

#include "Timestamp.h"
#include "noncopyable.h"

#include <functional>
#include <memory>

class EventLoop;

class Channel : noncopyable {
public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;

  Channel(EventLoop *loop, int fd);
  ~Channel();

  void handleEvent(Timestamp receiveTime);

  void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

  int fd() const { return fd_; }
  int events() const { return events_; }
  void setRevents(int revents) { revents_ = revents; }

  void enableReading();
  void disableReading();

  void enableWriting();
  void disableWriting();

  void disableAll();

  bool isNoneEvent() const;
  bool isWriting() const;
  bool isReading() const;

  int index() const { return index_; }
  void setIndex(int index) { index_ = index; }

  EventLoop *ownerLoop() { return loop_; }
  void remove();

private:
  void update();
  void handleEventWithGuard(Timestamp receiveTime);

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop *loop_;
  const int fd_;
  int events_;
  int revents_;
  int index_;

  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};
