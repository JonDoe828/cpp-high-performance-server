#include "Buffer.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <unistd.h>

TEST_CASE("Buffer appends and retrieves data") {
  Buffer buffer;

  REQUIRE(buffer.readableBytes() == 0);
  REQUIRE(buffer.writableBytes() == Buffer::kInitialSize);
  REQUIRE(buffer.prependableBytes() == Buffer::kCheapPrepend);

  buffer.append("hello", 5);
  REQUIRE(buffer.readableBytes() == 5);
  REQUIRE(buffer.retrieveAsString(2) == "he");
  REQUIRE(buffer.retrieveAllAsString() == "llo");
  REQUIRE(buffer.readableBytes() == 0);
}

TEST_CASE("Buffer finds CRLF boundaries") {
  Buffer buffer;
  std::string request = "GET / HTTP/1.1\r\nHost: local\r\n";
  buffer.append(request);

  const char *first = buffer.findCRLF();
  REQUIRE(first != nullptr);
  REQUIRE(std::string(buffer.peek(), first) == "GET / HTTP/1.1");

  buffer.retrieveUntil(first + 2);
  const char *second = buffer.findCRLF();
  REQUIRE(second != nullptr);
  REQUIRE(std::string(buffer.peek(), second) == "Host: local");
}

TEST_CASE("Buffer reuses prependable space before growing") {
  Buffer buffer(8);
  buffer.append("abcdefgh", 8);
  REQUIRE(buffer.writableBytes() == 0);

  REQUIRE(buffer.retrieveAsString(5) == "abcde");
  buffer.append("ijklm", 5);

  REQUIRE(buffer.retrieveAllAsString() == "fghijklm");
}

TEST_CASE("Buffer reads from file descriptor") {
  int fds[2];
  REQUIRE(::pipe(fds) == 0);

  std::string payload(2048, 'x');
  REQUIRE(::write(fds[1], payload.data(), payload.size()) ==
          static_cast<ssize_t>(payload.size()));

  Buffer buffer(16);
  int savedErrno = 0;
  REQUIRE(buffer.readFd(fds[0], &savedErrno) ==
          static_cast<ssize_t>(payload.size()));
  REQUIRE(savedErrno == 0);
  REQUIRE(buffer.readableBytes() == payload.size());
  REQUIRE(buffer.retrieveAllAsString() == payload);

  ::close(fds[0]);
  ::close(fds[1]);
}
