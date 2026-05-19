#include "Callbacks.h"
#include "Buffer.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Callbacks expose TcpConnection callback signatures") {
  bool connected = false;
  bool messaged = false;

  ConnectionCallback connectionCallback =
      [&](const TcpConnectionPtr &conn) {
        REQUIRE(!conn);
        connected = true;
      };

  MessageCallback messageCallback =
      [&](const TcpConnectionPtr &conn, Buffer *buffer, Timestamp receiveTime) {
        REQUIRE(!conn);
        REQUIRE(buffer != nullptr);
        REQUIRE(receiveTime.valid());
        messaged = true;
      };

  Buffer buffer;
  connectionCallback(TcpConnectionPtr());
  messageCallback(TcpConnectionPtr(), &buffer, Timestamp::now());

  REQUIRE(connected);
  REQUIRE(messaged);
}
