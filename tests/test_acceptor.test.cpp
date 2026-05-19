#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"

#include <catch2/catch_test_macros.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

TEST_CASE("Acceptor accepts a loopback client connection") {
  EventLoop loop;
  InetAddress listenAddr(0, "127.0.0.1");
  Acceptor acceptor(&loop, listenAddr);

  int acceptedFd = -1;
  InetAddress acceptedPeer;
  acceptor.setNewConnectionCallback(
      [&](int sockfd, const InetAddress &peerAddr) {
        acceptedFd = sockfd;
        acceptedPeer = peerAddr;
        loop.quit();
      });
  acceptor.listen();

  sockaddr_in localAddr {};
  socklen_t localLen = sizeof(localAddr);
  REQUIRE(::getsockname(acceptor.fd(), reinterpret_cast<sockaddr *>(&localAddr),
                        &localLen) == 0);
  REQUIRE(::ntohs(localAddr.sin_port) != 0);

  int clientFd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  REQUIRE(clientFd >= 0);

  sockaddr_in serverAddr {};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = localAddr.sin_port;
  REQUIRE(::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) == 1);
  REQUIRE(::connect(clientFd, reinterpret_cast<sockaddr *>(&serverAddr),
                    sizeof(serverAddr)) == 0);

  loop.loop();

  REQUIRE(acceptedFd >= 0);
  REQUIRE(acceptedPeer.toIp() == "127.0.0.1");

  ::close(acceptedFd);
  ::close(clientFd);
}
