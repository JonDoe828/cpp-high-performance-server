#include "Socket.h"
#include "InetAddress.h"
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

Socket::Socket(int sockfd) : sockfd_(sockfd) {}

Socket::~Socket() { ::close(sockfd_); }

int Socket::fd() const { return sockfd_; }

void Socket::bindAddress(const InetAddress &localaddr) {
  ::bind(sockfd_, reinterpret_cast<const sockaddr *>(localaddr.getSockAddr()),
         sizeof(sockaddr_in));
}

void Socket::listen() { ::listen(sockfd_, 1024); }

int Socket::accept(InetAddress *peeraddr) {
  sockaddr_in addr{};
  socklen_t len = sizeof(addr);

  int connfd = ::accept4(sockfd_, reinterpret_cast<sockaddr *>(&addr), &len,
                         SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd >= 0) {
    peeraddr->setSockAddr(addr);
  }
  return connfd;
}

void Socket::shutdownWrite() { ::shutdown(sockfd_, SHUT_WR); }

void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}