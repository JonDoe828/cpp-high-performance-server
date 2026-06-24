#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"

#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace {
const size_t kDefaultHighWaterMark = 64 * 1024 * 1024;
} // namespace

TcpConnection::TcpConnection(EventLoop *loop, std::string name, int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(loop),
      name_(std::move(name)),
      state_(kConnecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(kDefaultHighWaterMark) {
  channel_->setReadCallback(
      [this](Timestamp receiveTime) { handleRead(receiveTime); });
  channel_->setWriteCallback([this] { handleWrite(); });
  channel_->setCloseCallback([this] { handleClose(); });
  channel_->setErrorCallback([this] { handleError(); });
}

TcpConnection::~TcpConnection() = default;

EventLoop *TcpConnection::getLoop() const { return loop_; }

const std::string &TcpConnection::name() const { return name_; }

const InetAddress &TcpConnection::localAddress() const { return localAddr_; }

const InetAddress &TcpConnection::peerAddress() const { return peerAddr_; }

bool TcpConnection::connected() const { return state_ == kConnected; }

bool TcpConnection::disconnected() const { return state_ == kDisconnected; }

void TcpConnection::send(const std::string &message) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message.data(), message.size());
    } else {
      loop_->runInLoop(
          [this, message] { sendInLoop(message.data(), message.size()); });
    }
  }
}

void TcpConnection::send(Buffer *message) {
  if (state_ == kConnected) {
    std::string bytes(message->peek(), message->readableBytes());
    message->retrieveAll();
    send(bytes);
  }
}

void TcpConnection::shutdown() {
  if (state_ == kConnected) {
    setState(kDisconnecting);
    loop_->runInLoop([this] { shutdownInLoop(); });
  }
}

void TcpConnection::setTcpNoDelay(bool on) {
  socket_->setTcpNoDelay(on);
}

void TcpConnection::setConnectionCallback(ConnectionCallback cb) {
  connectionCallback_ = std::move(cb);
}

void TcpConnection::setMessageCallback(MessageCallback cb) {
  messageCallback_ = std::move(cb);
}

void TcpConnection::setWriteCompleteCallback(WriteCompleteCallback cb) {
  writeCompleteCallback_ = std::move(cb);
}

void TcpConnection::setHighWaterMarkCallback(HighWaterMarkCallback cb,
                                             size_t highWaterMark) {
  highWaterMarkCallback_ = std::move(cb);
  highWaterMark_ = highWaterMark;
}

void TcpConnection::setCloseCallback(CloseCallback cb) {
  closeCallback_ = std::move(cb);
}

Buffer *TcpConnection::inputBuffer() { return &inputBuffer_; }

Buffer *TcpConnection::outputBuffer() { return &outputBuffer_; }

void TcpConnection::connectEstablished() {
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading();

  if (connectionCallback_) {
    connectionCallback_(shared_from_this());
  }
}

void TcpConnection::connectDestroyed() {
  if (state_ == kConnected || state_ == kDisconnecting) {
    setState(kDisconnected);
    channel_->disableAll();
    if (connectionCallback_) {
      connectionCallback_(shared_from_this());
    }
  }
  channel_->remove();
}

void TcpConnection::setState(StateE state) { state_ = state; }

void TcpConnection::handleRead(Timestamp receiveTime) {
  int savedErrno = 0;
  const ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);

  if (n > 0) {
    if (messageCallback_) {
      messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
  } else if (n == 0) {
    handleClose();
  } else if (savedErrno != EAGAIN && savedErrno != EWOULDBLOCK) {
    errno = savedErrno;
    LOG_ERROR << "TcpConnection::handleRead name=" << name_
              << " failed: " << std::strerror(savedErrno);
    handleError();
  }
}

void TcpConnection::handleWrite() {
  if (!channel_->isWriting()) {
    return;
  }

  const ssize_t n =
      ::send(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes(),
             MSG_NOSIGNAL);
  if (n > 0) {
    outputBuffer_.retrieve(static_cast<size_t>(n));
    if (outputBuffer_.readableBytes() == 0) {
      channel_->disableWriting();
      if (writeCompleteCallback_) {
        writeCompleteCallback_(shared_from_this());
      }
      if (state_ == kDisconnecting) {
        shutdownInLoop();
      }
    }
  } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
    LOG_ERROR << "TcpConnection::handleWrite name=" << name_
              << " failed: " << std::strerror(errno);
  }
}

void TcpConnection::handleClose() {
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr guard(shared_from_this());
  if (connectionCallback_) {
    connectionCallback_(guard);
  }
  if (closeCallback_) {
    closeCallback_(guard);
  }
}

void TcpConnection::handleError() {
  int socketError = 0;
  socklen_t optionLen = sizeof(socketError);
  if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &socketError,
                   &optionLen) < 0) {
    socketError = errno;
  }
  LOG_ERROR << "TcpConnection::handleError name=" << name_
            << " error=" << socketError << ' ' << std::strerror(socketError);
}

void TcpConnection::sendInLoop(const void *data, size_t len) {
  if (state_ == kDisconnected) {
    return;
  }

  ssize_t written = 0;
  size_t remaining = len;
  bool faultError = false;

  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    written = ::send(channel_->fd(), data, len, MSG_NOSIGNAL);
    if (written >= 0) {
      remaining = len - static_cast<size_t>(written);
      if (remaining == 0 && writeCompleteCallback_) {
        writeCompleteCallback_(shared_from_this());
      }
    } else {
      written = 0;
      if (errno != EWOULDBLOCK && errno != EAGAIN) {
        LOG_ERROR << "TcpConnection::sendInLoop name=" << name_
                  << " failed: " << std::strerror(errno);
        if (errno == EPIPE || errno == ECONNRESET) {
          faultError = true;
        }
      }
    }
  }

  if (!faultError && remaining > 0) {
    const size_t oldLength = outputBuffer_.readableBytes();
    const size_t newLength = oldLength + remaining;
    if (newLength >= highWaterMark_ && oldLength < highWaterMark_ &&
        highWaterMarkCallback_) {
      highWaterMarkCallback_(shared_from_this(), newLength);
    }

    const char *bytes = static_cast<const char *>(data);
    outputBuffer_.append(bytes + written, remaining);
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdownInLoop() {
  if (!channel_->isWriting()) {
    socket_->shutdownWrite();
  }
}
