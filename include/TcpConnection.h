#pragma once

#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "noncopyable.h"

#include <memory>
#include <string>

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
public:
  TcpConnection(EventLoop *loop, std::string name, int sockfd,
                const InetAddress &localAddr,
                const InetAddress &peerAddr);
  ~TcpConnection();

  EventLoop *getLoop() const;
  const std::string &name() const;
  const InetAddress &localAddress() const;
  const InetAddress &peerAddress() const;

  bool connected() const;
  bool disconnected() const;

  void send(const std::string &message);
  void send(Buffer *message);
  void shutdown();
  void setTcpNoDelay(bool on);

  void setConnectionCallback(ConnectionCallback cb);
  void setMessageCallback(MessageCallback cb);
  void setWriteCompleteCallback(WriteCompleteCallback cb);
  void setHighWaterMarkCallback(HighWaterMarkCallback cb,
                                size_t highWaterMark);
  void setCloseCallback(CloseCallback cb);

  Buffer *inputBuffer();
  Buffer *outputBuffer();

  void connectEstablished();
  void connectDestroyed();

private:
  enum StateE {
    kDisconnected,
    kConnecting,
    kConnected,
    kDisconnecting,
  };

  void setState(StateE state);
  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();
  void sendInLoop(const void *data, size_t len);
  void shutdownInLoop();

  EventLoop *loop_;
  const std::string name_;
  StateE state_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddress localAddr_;
  const InetAddress peerAddr_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;
  CloseCallback closeCallback_;
  size_t highWaterMark_;

  Buffer inputBuffer_;
  Buffer outputBuffer_;
};
