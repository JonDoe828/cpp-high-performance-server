#pragma once

#include "Callbacks.h"
#include "noncopyable.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

class Acceptor;
class EventLoop;
class EventLoopThreadPool;
class InetAddress;

class TcpServer : noncopyable {
public:
  TcpServer(EventLoop *loop, const InetAddress &listenAddr, std::string name);
  ~TcpServer();

  EventLoop *getLoop() const;
  const std::string &name() const;
  uint16_t port() const;

  void setThreadNum(int numThreads);
  void setConnectionCallback(ConnectionCallback cb);
  void setMessageCallback(MessageCallback cb);
  void setWriteCompleteCallback(WriteCompleteCallback cb);

  void start();

private:
  using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

  void newConnection(int sockfd, const InetAddress &peerAddr);
  void removeConnection(const TcpConnectionPtr &connection);
  void removeConnectionInLoop(const TcpConnectionPtr &connection);

  EventLoop *loop_;
  const std::string name_;
  std::unique_ptr<Acceptor> acceptor_;
  std::shared_ptr<EventLoopThreadPool> threadPool_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  std::atomic_bool started_;
  int nextConnectionId_;
  ConnectionMap connections_;
};
