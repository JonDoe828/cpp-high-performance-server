#include "InetAddress.h"
#include "Logger.h"

#include <cstdint>
#include <cstring>
#include <netinet/in.h>

#include <sys/socket.h>

InetAddress::InetAddress(uint16_t port, std::string ip) {
  std::memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;

  addr_.sin_port = htons(port);
  if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
    Logger::error("InetAddress invalid ip: " + ip);
  }
}

InetAddress::InetAddress(const sockaddr_in &addr) : addr_(addr) {}

std::string InetAddress::toIp() const {
  char buf[64] = {0};
  ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
  return buf;
}

std::string InetAddress::toIpPort() const {
  return toIp() + ":" + std::to_string(toPort());
}

uint16_t InetAddress::toPort() const { return ::ntohs(addr_.sin_port); }

const sockaddr_in *InetAddress::getSockAddr() const { return &addr_; }
void InetAddress::setSockAddr(const sockaddr_in &addr) { addr_ = addr; }
