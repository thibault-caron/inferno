#ifndef I_POLLER_HPP
#define I_POLLER_HPP

#include <cstdint>
#include <vector>

// Platform-agnostic event flags — the Reactor only ever sees these
// namespace PollEvents {
// constexpr std::uint32_t READ = 1 << 0;   // data available to read
// constexpr std::uint32_t WRITE = 1 << 1;  // ready to write
// constexpr std::uint32_t ERROR = 1 << 2;  // error on fd
// }  // namespace PollEvents

enum class WatchFlags : std::uint32_t {
  READ = 1 << 0,
  WRITE = 1 << 1,
  ERROR = 1 << 2,
};

inline WatchFlags operator|(WatchFlags a, WatchFlags b) {
    return static_cast<WatchFlags>(
        static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

inline bool operator&(WatchFlags a, WatchFlags b) {
    return (static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b)) != 0;
}

struct ReadyEvent {
  int fileDescriptor;
  bool readable;  // EPOLLIN equivalent
  bool error;     // EPOLLERR equivalent
};

class IPoller {
 public:
  virtual ~IPoller() = default;

  virtual bool add(int fileDescriptor, WatchFlags events) = 0;  // watch this fileDescriptor
  virtual bool remove(int fileDescriptor) = 0;                     // stop watching this fd
  virtual int wait(std::vector<ReadyEvent>& readyEvents,
                   int timeoutMs) = 0;  // block until something is ready
};

#endif