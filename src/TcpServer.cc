#include "TcpServer.h"

#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <utility>

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
                     std::string name)
    : loop_(loop),
      name_(std::move(name)),
      acceptor_(new Acceptor(loop, listenAddr)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      started_(false),
      nextConnectionId_(1) {
  acceptor_->setNewConnectionCallback(
      [this](int sockfd, const InetAddress &peerAddr) {
        newConnection(sockfd, peerAddr);
      });
}

TcpServer::~TcpServer() {
  for (ConnectionMap::value_type &entry : connections_) {
    TcpConnectionPtr connection = entry.second;
    entry.second.reset();
    connection->getLoop()->runInLoop(
        [connection] { connection->connectDestroyed(); });
  }
}

EventLoop *TcpServer::getLoop() const { return loop_; }

const std::string &TcpServer::name() const { return name_; }

uint16_t TcpServer::port() const {
  sockaddr_in localAddr {};
  socklen_t addressLength = sizeof(localAddr);
  if (::getsockname(acceptor_->fd(), reinterpret_cast<sockaddr *>(&localAddr),
                    &addressLength) < 0) {
    LOG_ERROR << "TcpServer::port getsockname failed: "
              << std::strerror(errno);
    return 0;
  }
  return ::ntohs(localAddr.sin_port);
}

void TcpServer::setThreadNum(int numThreads) {
  threadPool_->setThreadNum(numThreads);
}

void TcpServer::setConnectionCallback(ConnectionCallback cb) {
  connectionCallback_ = std::move(cb);
}

void TcpServer::setMessageCallback(MessageCallback cb) {
  messageCallback_ = std::move(cb);
}

void TcpServer::setWriteCompleteCallback(WriteCompleteCallback cb) {
  writeCompleteCallback_ = std::move(cb);
}

void TcpServer::start() {
  bool expected = false;
  if (!started_.compare_exchange_strong(expected, true)) {
    return;
  }

  threadPool_->start();
  loop_->runInLoop([this] { acceptor_->listen(); });
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
  EventLoop *ioLoop = threadPool_->getNextLoop();
  const std::string connectionName =
      name_ + "-" + peerAddr.toIpPort() + "#" +
      std::to_string(nextConnectionId_++);

  sockaddr_in localSocketAddr {};
  socklen_t addressLength = sizeof(localSocketAddr);
  if (::getsockname(sockfd, reinterpret_cast<sockaddr *>(&localSocketAddr),
                    &addressLength) < 0) {
    LOG_ERROR << "TcpServer::newConnection getsockname failed: "
              << std::strerror(errno);
  }
  InetAddress localAddr(localSocketAddr);

  TcpConnectionPtr connection = std::make_shared<TcpConnection>(
      ioLoop, connectionName, sockfd, localAddr, peerAddr);
  connections_[connectionName] = connection;

  connection->setConnectionCallback(connectionCallback_);
  connection->setMessageCallback(messageCallback_);
  connection->setWriteCompleteCallback(writeCompleteCallback_);
  connection->setCloseCallback(
      [this](const TcpConnectionPtr &closedConnection) {
        removeConnection(closedConnection);
      });

  ioLoop->runInLoop([connection] { connection->connectEstablished(); });
}

void TcpServer::removeConnection(const TcpConnectionPtr &connection) {
  loop_->runInLoop(
      [this, connection] { removeConnectionInLoop(connection); });
}

void TcpServer::removeConnectionInLoop(
    const TcpConnectionPtr &connection) {
  const size_t erased = connections_.erase(connection->name());
  if (erased == 0) {
    LOG_WARN << "TcpServer::removeConnection unknown connection: "
             << connection->name();
    return;
  }

  EventLoop *ioLoop = connection->getLoop();
  ioLoop->queueInLoop([connection] { connection->connectDestroyed(); });
}
