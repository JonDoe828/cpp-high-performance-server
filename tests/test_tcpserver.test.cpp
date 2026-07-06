#include "Buffer.h"
#include "CurrentThread.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include "TcpServer.h"

#include <catch2/catch_test_macros.hpp>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <string>
#include <thread>

TEST_CASE("TcpServer manages a loopback echo connection") {
  EventLoop loop;
  TcpServer server(&loop, InetAddress(0, "127.0.0.1"), "echo");

  std::atomic_int connectionEvents(0);
  std::atomic_int callbackTid(0);
  std::atomic_bool messageReceived(false);
  server.setConnectionCallback([&](const TcpConnectionPtr &connection) {
    ++connectionEvents;
    if (connection->connected()) {
      callbackTid = CurrentThread::tid();
    } else {
      connection->getLoop()->queueInLoop([&loop] {
        loop.queueInLoop([&loop] { loop.quit(); });
      });
    }
  });
  server.setMessageCallback(
      [&](const TcpConnectionPtr &connection, Buffer *buffer, Timestamp) {
        messageReceived = true;
        connection->send(buffer->retrieveAllAsString());
        connection->shutdown();
      });

  server.setThreadNum(1);
  server.start();
  REQUIRE(server.port() != 0);

  std::atomic_bool clientSucceeded(false);
  std::thread client([&] {
    int clientFd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (clientFd < 0) {
      loop.quit();
      return;
    }

    sockaddr_in serverAddr {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = ::htons(server.port());
    ::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (::connect(clientFd, reinterpret_cast<sockaddr *>(&serverAddr),
                  sizeof(serverAddr)) == 0 &&
        ::write(clientFd, "ping", 4) == 4) {
      char response[4] = {};
      const ssize_t bytesRead = ::read(clientFd, response, sizeof(response));
      clientSucceeded =
          bytesRead == 4 && std::string(response, sizeof(response)) == "ping";
    }

    ::close(clientFd);
    if (!clientSucceeded) {
      loop.quit();
    }
  });

  loop.loop();
  client.join();

  REQUIRE(clientSucceeded);
  REQUIRE(messageReceived);
  REQUIRE(connectionEvents == 2);
  REQUIRE(callbackTid != CurrentThread::tid());
}
