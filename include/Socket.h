#pragma once
#include "noncopyable.h"
class InetAddress;
class Socket : noncopyable {
public:
  explicit Socket(int sockfd);
  ~Socket();

  int fd() const;

  void bindAddress(const InetAddress &localaddr);
  void listen();
  int accept(InetAddress *peeraddr);

  void shutdownWrite();

  void setTcpNoDelay(bool on);
  void setReuseAddr(bool on);
  void setReusePort(bool on);
  void setKeepAlive(bool on);

private:
  const int
      sockfd_; // Socket 是 fd 封装,socket 本质就是一个文件描述符，类型是 in
};