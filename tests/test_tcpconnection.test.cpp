#include "EventLoop.h"
#include "InetAddress.h"
#include "TcpConnection.h"

#include <catch2/catch_test_macros.hpp>

#include <sys/socket.h>
#include <unistd.h>

#include <memory>
#include <string>

TEST_CASE("TcpConnection receives and sends data") {
  int fds[2];
  REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0,
                       fds) == 0);

  EventLoop loop;
  InetAddress localAddr(8000);
  InetAddress peerAddr(9000);
  TcpConnectionPtr connection = std::make_shared<TcpConnection>(
      &loop, "test-connection", fds[0], localAddr, peerAddr);

  int connectionEvents = 0;
  bool messageReceived = false;
  bool writeCompleted = false;

  connection->setConnectionCallback([&](const TcpConnectionPtr &) {
    ++connectionEvents;
  });
  connection->setWriteCompleteCallback([&](const TcpConnectionPtr &) {
    writeCompleted = true;
  });
  connection->setMessageCallback(
      [&](const TcpConnectionPtr &conn, Buffer *buffer, Timestamp receiveTime) {
        REQUIRE(receiveTime.valid());
        REQUIRE(buffer->retrieveAllAsString() == "ping");
        messageReceived = true;
        conn->send("pong");
        loop.quit();
      });

  connection->connectEstablished();
  REQUIRE(connection->connected());
  REQUIRE(connectionEvents == 1);

  REQUIRE(::write(fds[1], "ping", 4) == 4);
  loop.loop();

  char response[4] = {};
  REQUIRE(::read(fds[1], response, sizeof(response)) == 4);
  REQUIRE(std::string(response, sizeof(response)) == "pong");
  REQUIRE(messageReceived);
  REQUIRE(writeCompleted);

  connection->shutdown();
  REQUIRE(::read(fds[1], response, sizeof(response)) == 0);

  connection->connectDestroyed();
  REQUIRE(connection->disconnected());
  REQUIRE(connectionEvents == 2);

  connection.reset();
  ::close(fds[1]);
}

TEST_CASE("TcpConnection reports peer close") {
  int fds[2];
  REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0,
                       fds) == 0);

  EventLoop loop;
  InetAddress localAddr(8001);
  InetAddress peerAddr(9001);
  TcpConnectionPtr connection = std::make_shared<TcpConnection>(
      &loop, "closing-connection", fds[0], localAddr, peerAddr);

  int connectionEvents = 0;
  bool closeCalled = false;
  connection->setConnectionCallback([&](const TcpConnectionPtr &) {
    ++connectionEvents;
  });
  connection->setCloseCallback([&](const TcpConnectionPtr &conn) {
    REQUIRE(conn->disconnected());
    closeCalled = true;
    loop.quit();
  });

  connection->connectEstablished();
  ::close(fds[1]);
  loop.loop();

  REQUIRE(closeCalled);
  REQUIRE(connection->disconnected());
  REQUIRE(connectionEvents == 2);

  connection->connectDestroyed();
  connection.reset();
}
