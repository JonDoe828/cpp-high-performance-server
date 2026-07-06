#include "InetAddress.h"
#include "Logger.h"

#include <cstdint>
#include <cstring>
#include <netinet/in.h>

#include <sys/socket.h>
// InetAddress:负责保存 IP + port，并处理字符串、端口、字节序、sockaddr_in
// 之间的转换
InetAddress::InetAddress(uint16_t port, std::string ip) {
  std::memset(&addr_, 0,
              sizeof(addr_)); // memset：把结构体清零，避免里面有随机脏数据
  addr_.sin_family = AF_INET; // 地址族，比如 AF_INET 表示 IPv4

  addr_.sin_port = htons(
      port); // 端口，网络字节序. htons(port)：把主机字节序端口转成网络字节序
  if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <=
      0) { // sin_addr: IP，网络字节序
    // inet_addr(ip.c_str())：把 "127.0.0.1" 这种字符串 IP 转成内核要的二进制 IP
    Logger::error("InetAddress invalid ip: " + ip);
  }
}

InetAddress::InetAddress(const sockaddr_in &addr) : addr_(addr) {}

std::string InetAddress::toIp() const {
  char buf[64] = {0};
  ::inet_ntop(AF_INET, &addr_.sin_addr, buf,
              sizeof buf); // inet_addr 的反方向，把二进制 IP 转成字符串，比如
                           // "127.0.0.1"
  return buf;
}

std::string InetAddress::toIpPort() const {
  return toIp() + ":" + std::to_string(toPort());
}

uint16_t InetAddress::toPort() const { return ::ntohs(addr_.sin_port); }

const sockaddr_in *InetAddress::getSockAddr() const { return &addr_; }
void InetAddress::setSockAddr(const sockaddr_in &addr) { addr_ = addr; }
