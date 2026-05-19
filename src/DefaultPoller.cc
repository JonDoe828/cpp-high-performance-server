#include "EPollPoller.h"
#include "Poller.h"

Poller *Poller::newDefaultPoller(EventLoop *loop) {
  return new EPollPoller(loop);
}
