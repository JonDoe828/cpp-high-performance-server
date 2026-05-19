#pragma once

#include "Channel.h"
#include "Socket.h"
#include "noncopyable.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable {
public:
  using NewConnectionCallback =
      std::function<void(int sockfd, const InetAddress &peerAddr)>;

  Acceptor(EventLoop *loop, const InetAddress &listenAddr);
  ~Acceptor();

  void setNewConnectionCallback(NewConnectionCallback cb);
  void listen();
  bool listening() const;

  int fd() const;

private:
  void handleRead();

  EventLoop *loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool listening_;
};
