#include "agent_loop.hpp"

#include <optional>
#include <sstream>
#include <vector>

AgentLoop::AgentLoop(IPoller& poller, AgentDispatcher& dispatcher,
                     std::string host, std::uint16_t port, int heartbeatMs,
                     int retryMs)
    : poller_(poller),
      dispatcher_(dispatcher),
      host_(std::move(host)),
      port_(port),
      heartbeatMs_(heartbeatMs),
      retryMs_(retryMs),
      running_(true),
      connected_(false),
      session_() {}

// ─── effectiveTimeout
// ─────────────────────────────────────────────────────────
//
// The single loop uses two different timeouts depending on state:
//  - connected:     heartbeatMs_ — if server is silent this long, send
//  HEALTHCHECK
//  - disconnected:  retryMs_     — wait this long before the next connect
//  attempt
//
// When disconnected, no fds are registered in the poller, so poll() with nfds=0
// blocks for retryMs_ then returns 0 — triggering tryReconnect().
// This replaces ::sleep() entirely.
int AgentLoop::effectiveTimeout() const {
  return connected_ ? heartbeatMs_ : retryMs_;
}

// ─── run
// ──────────────────────────────────────────────────────────────────────
//
// Single event loop — no nested loops, no ::sleep(), no break/continue.
//
// When !connected_: poller has no fds, wait() blocks for retryMs_ then
// returns 0. We ignore n entirely and call tryReconnect().
//
// When connected_: poller watches the socket fd.
//   n < 0  → poll error
//   n == 0 → timeout (heartbeat)
//   n > 0  → events to process
void AgentLoop::run() {
  while (running_) {
    std::vector<ReadyEvent> events;
    const int pollResult = poller_.wait(events, effectiveTimeout());

    if (!connected_) {
      tryReconnect();
    } else if (pollResult < 0) {
      logger_.error("poll error");
      onError();
    } else if (pollResult == 0) {
      onTimeout();
    } else {
      for (const ReadyEvent& event : events) {
        if (event.error) {
          onError();
        } else if (event.readable) {
          onReadable();
        }
      }
    }
  }
}

// ─── tryReconnect
// ─────────────────────────────────────────────────────────────
//
// Called every retryMs_ while disconnected.
// resetSession() creates a fresh socket internally and clears buffer/state.
// agentInfo_ survives — hostname/arch/os are only queried once per agent
// lifetime; sendRegister() reuses whatever is already in agentInfo_.
//
// On success: fd is added to the poller and connected_ = true.
//             The next wait() uses heartbeatMs_ and watches the fd.
// On failure: just log. The loop will call tryReconnect() again after
//             the next retryMs_ timeout. Nothing to clean up.
void AgentLoop::tryReconnect() {
  session_.resetSession();

  if (session_.socket && session_.socket->connect(host_, port_)) {
    dispatcher_.sendRegister(session_);

    if (session_.getRegistered_() == RegisterState::SENT) {
      poller_.add(session_.socket->getFd(), WatchFlags::READ);
      connected_ = true;
      std::ostringstream msg;
      msg << "connected to " << host_ << ":" << port_;
      logger_.info(msg.str());
    } else {
      // REGISTER serialization/send failed — socket is useless, close it.
      // No poller.remove() needed since we never called poller.add().
      session_.socket->close();
      logger_.warn("REGISTER send failed, will retry");
    }
  } else {
    std::ostringstream msg;
    msg << "connect failed to " << host_ << ":" << port_;
    logger_.warn(msg.str());
  }
}

// ─── onTimeout
// ────────────────────────────────────────────────────────────────
//
// poll() returned 0 while connected — server has been silent for heartbeatMs_.
// Future: build and send a HEALTHCHECK frame here so the server knows we
// are still alive. For now, just log.
void AgentLoop::onTimeout() {
  logger_.info("heartbeat timeout — HEALTHCHECK placeholder");
}

// ─── onReadable
// ───────────────────────────────────────────────────────────────
//
// Data available on the socket. Two outcomes:
//
//  recv failed / EOF (bytesTransferred <= 0):
//    Clean TCP close or socket error — go through onDisconnect().
//
//  recv OK:
//    Drain every complete frame out of the buffer.
//    After each handleFrame(), check session_.isValid(): the dispatcher
//    may have closed the socket in response to a DISCONNECT frame,
//    which would make further reads pointless.
void AgentLoop::onReadable() {
  const SocketResult result = session_.receiveIntoBuffer();

  if (!result.ok() || result.bytesTransferred <= 0) {
    std::ostringstream msg;
    msg << "recv returned bytes=" << result.bytesTransferred
        << ", disconnecting";
    logger_.warn(msg.str());
    onDisconnect();
  } else {
    // std::optional<Frame> frame = session_.tryExtractFrame();
    std::optional<Frame> frame;
    while (session_.isValid() && (frame = session_.tryExtractFrame())) {
      dispatcher_.handleFrame(session_, frame.value());
    //   frame = session_.tryExtractFrame();
    }

    if (!session_.isValid()) {
      onDisconnect();
    }
  }
}

// ─── onError
// ──────────────────────────────────────────────────────────────────
//
// poll() reported POLLERR or POLLNVAL. Not a clean close — something broke
// at the socket level. Treat identically to disconnection.
void AgentLoop::onError() {
  logger_.error("socket error reported by poll");
  onDisconnect();
}

// ─── onDisconnect
// ─────────────────────────────────────────────────────────────
//
// Single cleanup point for every disconnection path.
//
// Order matters:
//  1. poller_.remove() first — the fd value is still valid here.
//     Our Poller::remove() only touches fds_; it doesn't call close().
//  2. socket->close() — releases the OS fd.
//  3. connected_ = false — next loop iteration uses retryMs_ and calls
//     tryReconnect() instead of processing events.
void AgentLoop::onDisconnect() {
  poller_.remove(session_.socket->getFd());
  session_.socket->close();
  connected_ = false;
  logger_.info("disconnected — will retry in retryMs");
}