#include "Buffer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"
#include "TcpConnection.h"
#include "TcpServer.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
bool parseInteger(const char *text, int minValue, int maxValue, int *result) {
  char *end = nullptr;
  errno = 0;
  const long value = std::strtol(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value < minValue ||
      value > maxValue || value > INT_MAX) {
    return false;
  }

  *result = static_cast<int>(value);
  return true;
}

class EchoServer {
public:
  EchoServer(EventLoop *loop, const InetAddress &listenAddr, int threadCount)
      : server_(loop, listenAddr, "EchoServer") {
    server_.setConnectionCallback(
        [this](const TcpConnectionPtr &connection) {
          onConnection(connection);
        });
    server_.setMessageCallback(
        [this](const TcpConnectionPtr &connection, Buffer *buffer,
               Timestamp receiveTime) {
          onMessage(connection, buffer, receiveTime);
        });
    server_.setThreadNum(threadCount);
  }

  void start() { server_.start(); }

private:
  void onConnection(const TcpConnectionPtr &connection) {
    LOG_INFO << "connection " << (connection->connected() ? "UP " : "DOWN ")
             << connection->name() << " peer="
             << connection->peerAddress().toIpPort();
  }

  void onMessage(const TcpConnectionPtr &connection, Buffer *buffer,
                 Timestamp) {
    connection->send(buffer->retrieveAllAsString());
  }

  TcpServer server_;
};
} // namespace

int main(int argc, char *argv[]) {
  int port = 8080;
  int threadCount = 3;

  if (argc > 3 ||
      (argc >= 2 && !parseInteger(argv[1], 1, 65535, &port)) ||
      (argc == 3 && !parseInteger(argv[2], 0, 128, &threadCount))) {
    std::cerr << "Usage: " << argv[0] << " [port] [thread-count]\n";
    return EXIT_FAILURE;
  }

  EventLoop loop;
  InetAddress listenAddr(static_cast<uint16_t>(port), "0.0.0.0");
  EchoServer server(&loop, listenAddr, threadCount);

  LOG_INFO << "EchoServer listening on " << listenAddr.toIpPort()
           << " with " << threadCount << " I/O threads";
  server.start();
  loop.loop();
}
