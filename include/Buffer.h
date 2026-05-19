#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>
#include <sys/types.h>

class Buffer {
public:
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;

  explicit Buffer(size_t initialSize = kInitialSize);

  size_t readableBytes() const;
  size_t writableBytes() const;
  size_t prependableBytes() const;

  const char *peek() const;
  const char *findCRLF() const;
  const char *findCRLF(const char *start) const;

  void retrieve(size_t len);
  void retrieveUntil(const char *end);
  void retrieveAll();
  std::string retrieveAsString(size_t len);
  std::string retrieveAllAsString();

  void append(const std::string &str);
  void append(const char *data, size_t len);
  void append(const void *data, size_t len);

  void ensureWritableBytes(size_t len);
  char *beginWrite();
  const char *beginWrite() const;
  void hasWritten(size_t len);
  void unwrite(size_t len);

  void prepend(const void *data, size_t len);
  void shrink(size_t reserve);

  ssize_t readFd(int fd, int *savedErrno);

private:
  char *begin();
  const char *begin() const;
  void makeSpace(size_t len);

  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;
};
