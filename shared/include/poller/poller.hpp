#ifndef POLLER_HPP
#define POLLER_HPP

// poll() is POSIX — available on Linux and macOS out of the box.
// On Windows, WSAPoll() is the equivalent and uses the same struct/flags,
// so the only porting seam is the actual call site in the .cpp.
#ifdef _WIN32
  #include <winsock2.h>   // defines pollfd + WSAPoll
#else
  #include <poll.h>       // defines pollfd + poll
#endif

#include <vector>

#include "i_poller.hpp"

class Poller : public IPoller {
 public:
  bool add(int fileDescriptor, WatchFlags events) override;
  bool remove(int fileDescriptor) override;
  int wait(std::vector<ReadyEvent>& readyEvents, int timeoutMs) override;

 private:
  // The raw array that poll() reads and writes.
  // Each entry tracks one fd: what we want to watch (events)
  // and what actually happened (revents, filled by the kernel).
  std::vector<struct pollfd> fileDescriptors_;

  // Converts our WatchFlags bitmask into the native short that pollfd expects.
  // Kept private because nothing outside this class needs to know about
  // poll's native flags — that's exactly what IPoller is hiding.
  static short toNativeEvents(WatchFlags flags);

  // Finds the index of an fd inside fds_, or -1 if not found.
  // Used by both add() (duplicate guard) and remove() (lookup before erase).
  int indexOf(int fileDescriptor) const;

  int poll(int timeoutMs);
};

#endif  // POLLER_HPP