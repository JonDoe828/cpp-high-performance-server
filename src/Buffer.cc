#include "Buffer.h"

#include <cerrno>
#include <cstring>
#include <sys/uio.h>
#include <unistd.h>

namespace {
const char kCRLF[] = "\r\n";
} // namespace

Buffer::Buffer(size_t initialSize)
    : buffer_(kCheapPrepend + initialSize),
      readerIndex_(kCheapPrepend),
      writerIndex_(kCheapPrepend) {}

size_t Buffer::readableBytes() const { return writerIndex_ - readerIndex_; }

size_t Buffer::writableBytes() const { return buffer_.size() - writerIndex_; }

size_t Buffer::prependableBytes() const { return readerIndex_; }

const char *Buffer::peek() const { return begin() + readerIndex_; }

const char *Buffer::findCRLF() const { return findCRLF(peek()); }

const char *Buffer::findCRLF(const char *start) const {
  assert(peek() <= start);
  assert(start <= beginWrite());

  const char *crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
  return crlf == beginWrite() ? nullptr : crlf;
}

void Buffer::retrieve(size_t len) {
  assert(len <= readableBytes());
  if (len < readableBytes()) {
    readerIndex_ += len;
  } else {
    retrieveAll();
  }
}

void Buffer::retrieveUntil(const char *end) {
  assert(peek() <= end);
  assert(end <= beginWrite());
  retrieve(static_cast<size_t>(end - peek()));
}

void Buffer::retrieveAll() {
  readerIndex_ = kCheapPrepend;
  writerIndex_ = kCheapPrepend;
}

std::string Buffer::retrieveAsString(size_t len) {
  assert(len <= readableBytes());
  std::string result(peek(), len);
  retrieve(len);
  return result;
}

std::string Buffer::retrieveAllAsString() {
  return retrieveAsString(readableBytes());
}

void Buffer::append(const std::string &str) { append(str.data(), str.size()); }

void Buffer::append(const char *data, size_t len) {
  ensureWritableBytes(len);
  std::copy(data, data + len, beginWrite());
  hasWritten(len);
}

void Buffer::append(const void *data, size_t len) {
  append(static_cast<const char *>(data), len);
}

void Buffer::ensureWritableBytes(size_t len) {
  if (writableBytes() < len) {
    makeSpace(len);
  }
  assert(writableBytes() >= len);
}

char *Buffer::beginWrite() { return begin() + writerIndex_; }

const char *Buffer::beginWrite() const { return begin() + writerIndex_; }

void Buffer::hasWritten(size_t len) {
  assert(len <= writableBytes());
  writerIndex_ += len;
}

void Buffer::unwrite(size_t len) {
  assert(len <= readableBytes());
  writerIndex_ -= len;
}

void Buffer::prepend(const void *data, size_t len) {
  assert(len <= prependableBytes());
  readerIndex_ -= len;
  const char *d = static_cast<const char *>(data);
  std::copy(d, d + len, begin() + readerIndex_);
}

void Buffer::shrink(size_t reserve) {
  Buffer other;
  other.ensureWritableBytes(readableBytes() + reserve);
  other.append(peek(), readableBytes());
  buffer_.swap(other.buffer_);
  readerIndex_ = other.readerIndex_;
  writerIndex_ = other.writerIndex_;
}

ssize_t Buffer::readFd(int fd, int *savedErrno) {
  char extraBuf[65536];
  iovec vec[2];
  const size_t writable = writableBytes();

  vec[0].iov_base = begin() + writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extraBuf;
  vec[1].iov_len = sizeof(extraBuf);

  const int iovcnt = writable < sizeof(extraBuf) ? 2 : 1;
  const ssize_t n = ::readv(fd, vec, iovcnt);
  if (n < 0) {
    *savedErrno = errno;
  } else if (static_cast<size_t>(n) <= writable) {
    writerIndex_ += static_cast<size_t>(n);
  } else {
    writerIndex_ = buffer_.size();
    append(extraBuf, static_cast<size_t>(n) - writable);
  }
  return n;
}

char *Buffer::begin() { return buffer_.data(); }

const char *Buffer::begin() const { return buffer_.data(); }

void Buffer::makeSpace(size_t len) {
  if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
    buffer_.resize(writerIndex_ + len);
  } else {
    const size_t readable = readableBytes();
    std::copy(begin() + readerIndex_, begin() + writerIndex_,
              begin() + kCheapPrepend);
    readerIndex_ = kCheapPrepend;
    writerIndex_ = readerIndex_ + readable;
  }
}
