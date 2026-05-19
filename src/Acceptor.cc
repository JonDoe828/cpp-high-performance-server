#include "Acceptor.h"
#include "InetAddress.h"
#include "Logger.h"

#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace {
int createNonblockingSocket() {
  int sockfd =
      ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (sockfd < 0) {
    LOG_FATAL << "Acceptor::createNonblockingSocket failed: "
              << std::strerror(errno);
  }
  return sockfd;
}
} // namespace

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr)
    : loop_(loop),
      acceptSocket_(createNonblockingSocket()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listening_(false) {
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(true);
  acceptSocket_.bindAddress(listenAddr);
  acceptChannel_.setReadCallback([this](Timestamp) { handleRead(); });
}

Acceptor::~Acceptor() {
  if (listening_) {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
  }
}

void Acceptor::setNewConnectionCallback(NewConnectionCallback cb) {
  newConnectionCallback_ = std::move(cb);
}

void Acceptor::listen() {
  listening_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableReading();
}

bool Acceptor::listening() const { return listening_; }

int Acceptor::fd() const { return acceptSocket_.fd(); }

void Acceptor::handleRead() {
  InetAddress peerAddr;
  int connfd = acceptSocket_.accept(&peerAddr);
  if (connfd >= 0) {
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd, peerAddr);
    } else {
      ::close(connfd);
    }
  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    LOG_ERROR << "Acceptor::handleRead accept failed: " << std::strerror(errno);
  }
}
