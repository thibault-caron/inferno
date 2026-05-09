#include "poller/poller.hpp"

#include <poll.h>

// ─── indexOf
// ──────────────────────────────────────────────────────────────────
//
// Walks fds_ and returns the position of the entry whose .fd matches.
// Returns -1 when the fd isn't registered.
//
// Why int and not size_t?
//   Returning -1 as a sentinel is the clearest way to signal "not found"
//   without an std::optional.  Using size_t would force callers to compare
//   against fds_.size() or a magic value, which is less readable.
//
// Why not std::find_if?
//   Core constraints ban lambdas.  An index-based loop is the natural
//   replacement; it also gives us the integer index directly without a
//   subsequent std::distance call.
int Poller::indexOf(int fileDescriptor) const {
  for (int i = 0; i < static_cast<int>(fileDescriptors_.size()); ++i) {
    if (fileDescriptors_[static_cast<std::size_t>(i)].fd == fileDescriptor) {
      return i;
    }
  }
  return -1;
}

// ─── toNativeEvents
// ───────────────────────────────────────────────────────────
//
// Translates our platform-agnostic WatchFlags into the short bitmask that
// struct pollfd.events expects.
//
// poll() uses a different set of constants than epoll:
//   POLLIN  — there is data to read (≈ EPOLLIN)
//   POLLOUT — the fd is ready to write without blocking (≈ EPOLLOUT)
//   POLLERR — an error condition exists on the fd (≈ EPOLLERR)
//
// Note: POLLERR and POLLHUP are *always* reported in revents regardless of
// whether you include them in events.  We add POLLERR to events anyway for
// symmetry with Epoller and to make intent explicit.
//
// Note: POLLHUP (peer closed the connection) is not in WatchFlags because
// IPoller's ReadyEvent only exposes readable + error.  POLLHUP is treated
// as a read-hangup: when we see it in revents we set readable = true so the
// caller's recv() returns 0, which is the standard "connection closed" signal.
// That decision lives in wait(), not here.
short Poller::toNativeEvents(WatchFlags flags) {
  short result = 0;
  if (flags & WatchFlags::READ) result |= POLLIN;
  if (flags & WatchFlags::WRITE) result |= POLLOUT;
  if (flags & WatchFlags::ERROR) result |= POLLERR;
  return result;
}

// ─── add
// ──────────────────────────────────────────────────────────────────────
//
// Registers a new fd for watching.
//
// We guard against duplicates because poll() would then report that fd twice
// in the same wait() call, which would confuse the caller.  Trying to add an
// fd that's already watched is a programming error, so returning false (rather
// than silently succeeding) makes it detectable.
//
// struct pollfd zero-initialises revents to 0.  poll() will overwrite it on
// each call, so the initial value doesn't matter — but being explicit avoids
// stale data if someone inspects the struct in a debugger.
bool Poller::add(int fileDescriptor, WatchFlags events) {
  if (indexOf(fileDescriptor) != -1) {
    return false;  // already registered
  }

  struct pollfd entry{};
  entry.fd = fileDescriptor;
  entry.events = toNativeEvents(events);
  entry.revents = 0;

  fileDescriptors_.push_back(entry);
  return true;
}

// ─── remove
// ───────────────────────────────────────────────────────────────────
//
// Stops watching an fd and removes it from fds_.
//
// We use swap-and-pop rather than erase(iterator) to avoid shifting every
// element after the removed one.  Order inside fds_ doesn't matter to poll(),
// so this is safe and O(1) instead of O(n).
//
// If the fd isn't found we return false.  The caller (AgentLoop) can decide
// whether that's worth logging; it's not inherently fatal.
bool Poller::remove(int fileDescriptor) {
  const int index = indexOf(fileDescriptor);
  if (index == -1) {
    return false;  // wasn't registered
  }

  // Swap the target entry with the last one, then pop the last.
  // If the target IS the last entry the swap is a no-op and pop still works.
  const std::size_t idx = static_cast<std::size_t>(index);
  fileDescriptors_[idx] = fileDescriptors_.back();
  fileDescriptors_.pop_back();
  return true;
}

// ─── wait
// ─────────────────────────────────────────────────────────────────────
//
// Blocks until at least one watched fd is ready, or timeoutMs elapses.
//
// Return value mirrors poll() directly:
//   > 0  — number of fds that are ready (readyEvents will have that many
//   entries) = 0  — timeout elapsed, nothing was ready < 0  — poll() itself
//   failed (check errno); readyEvents is untouched
//
// Why do we *append* to readyEvents rather than clearing it first?
//   IPoller::wait's contract (matching Epoller) says "append".  The caller
//   owns the vector and decides when to clear it.  This lets the caller
//   accumulate events across multiple wait() calls if it ever needs to,
//   and avoids a hidden allocation on every call.
//
// POLLHUP handling:
//   POLLHUP means the remote end closed the connection.  It is not an error
//   per se — it's the normal TCP FIN path.  We surface it as readable = true
//   so the caller does a recv() that returns 0, which is the idiomatic POSIX
//   "EOF / connection closed" signal.  This keeps the caller's logic simple:
//   it only needs to check readable and then handle recv() == 0.
//
// POLLNVAL:
//   Means the fd is not open.  Treated as an error — something went wrong
//   before wait() was called (fd was closed without calling remove()).
int Poller::wait(std::vector<ReadyEvent>& readyEvents, int timeoutMs) {
  // if (fileDescriptors_.empty()) {
  //   return 0;  // nothing to watch; treat as timeout
  // }

  const int ready = poll(timeoutMs);

  if (ready <= 0) {
    return ready;  // timeout (0) or error (<0) — nothing to translate
  }

  for (const struct pollfd& entry : fileDescriptors_) {
    if (entry.revents != 0) {
      ReadyEvent event{};
      event.fileDescriptor = entry.fd;

      // readable: data available OR peer closed (POLLHUP → recv() returns 0)
      event.readable =
          (entry.revents & POLLIN) != 0 || (entry.revents & POLLHUP) != 0;

      // error: hard error or invalid fd
      event.error =
          (entry.revents & POLLERR) != 0 || (entry.revents & POLLNVAL) != 0;

      readyEvents.push_back(event);
    }
  }

  return ready;
}

int Poller::poll(int timeoutMs) {
#ifdef _WIN32
  if (fds_.empty()) {
    if (timeoutMs > 0) {
      ::Sleep(static_cast<DWORD>(timeoutMs));
    }
    return 0;
  }
  return ::WSAPoll(fileDescriptors_.data(),
                   static_cast<ULONG>(fileDescriptors_.size()), timeoutMs);
#else
  // nfds=0 is valid POSIX: blocks for timeoutMs, watches nothing.
  return ::poll(fileDescriptors_.data(),
                static_cast<nfds_t>(fileDescriptors_.size()), timeoutMs);
#endif
}