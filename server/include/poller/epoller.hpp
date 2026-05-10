#ifndef EPOLLER_HPP
#define EPOLLER_HPP

#include "poller/i_poller.hpp"

class Epoller : public IPoller {
 private:
  static constexpr int maxEvents_ = 64;  // TODO : get rid of magic number
  int pollerFileDescriptor_ = -1;
  static std::uint32_t toEpollEvents(WatchFlags events);

 public:
  Epoller();
  ~Epoller() override;
  Epoller(const Epoller&) = delete;
  Epoller& operator=(const Epoller&) = delete;

  bool add(int fileDescriptor, WatchFlags events);  // watch this fileDescriptor
  bool remove(int fileDescriptor);                  // stop watching this fileDescriptor
  int wait(std::vector<ReadyEvent>& readyEvents,
           int timeoutMs);  // block until something is ready
  bool isValid() const;
};

#endif