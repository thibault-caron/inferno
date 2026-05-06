#include "poller/epoller.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>

Epoller::Epoller() {
  pollerFileDescriptor_ = ::epoll_create1(0);
  if (pollerFileDescriptor_ == -1) {
    std::cerr << "[Epoller] epoll_create1 failed\n";
  }
}

Epoller::~Epoller() {
  if (pollerFileDescriptor_ != -1) {
    ::close(pollerFileDescriptor_);
    pollerFileDescriptor_ = -1;
  }
}

bool Epoller::isValid() const { return pollerFileDescriptor_ != -1; }

bool Epoller::add(int fileDescriptor, WatchFlags events) {
  epoll_event epollEvent{};
  // ev.events  = toEpollEvents(events);
  epollEvent.events = toEpollEvents(events);
  epollEvent.data.fd = fileDescriptor;
  return ::epoll_ctl(pollerFileDescriptor_, EPOLL_CTL_ADD, fileDescriptor,
                     &epollEvent) == 0;
}

bool Epoller::remove(int fileDescriptor) {
  // fourth argument can be nullptr on Linux kernel >= 2.6.9
  return ::epoll_ctl(pollerFileDescriptor_, EPOLL_CTL_DEL, fileDescriptor,
                     nullptr) == 0;
}

int Epoller::wait(std::vector<ReadyEvent>& readyEvents, int timeoutMs) {
  epoll_event epollEvents[maxEvents_];

  // nReady renaming option : eventCount, firedCount, readyCount
  const int readyCount =
      ::epoll_wait(pollerFileDescriptor_, epollEvents, maxEvents_, timeoutMs);

  if (readyCount <= 0) {
    // nReady == 0 → timeout, nothing ready
    // nReady == -1 → error (check errno if needed)
    return readyCount;
  }

  readyEvents.clear();
  for (int i = 0; i < readyCount; i++) {
    ReadyEvent event{};
    event.fileDescriptor = epollEvents[i].data.fd;
    event.readable = (epollEvents[i].events & EPOLLIN) != 0;
    event.error = (epollEvents[i].events & (EPOLLERR | EPOLLHUP)) != 0;
    readyEvents.push_back(event);
  }

  return readyCount;
}

// ── Private helpers ───────────────────────────────────────────
std::uint32_t Epoller::toEpollEvents(WatchFlags events) {
  std::uint32_t result = 0;
  if (events & WatchFlags::READ) result |= EPOLLIN;
  if (events & WatchFlags::WRITE) result |= EPOLLOUT;
  if (events & WatchFlags::ERROR) result |= EPOLLERR;
  return result;
}