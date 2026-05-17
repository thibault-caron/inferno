#ifndef SPY_SOCKET_HPP
#define SPY_SOCKET_HPP

#include <vector>

#include "socket/i_socket.hpp"

// SpySocket — a minimal stub that records every byte sent to it.
// No GMock, no matchers, no reference_wrapper gymnastics.
//
// Usage:
//   SpySocket spy;
//   AgentSession session(spy.makeUnique());
//   // ... call dispatcher ...
//   EXPECT_EQ(spy.messageType(), MessageType::COMMAND);
//
// Owns no fd. isValid() reflects close() so tests can assert shutdown.
class SpySocket : public ISocket {
 public:
  // Transfers ownership to an AgentSession while keeping spy accessible.
  // Works because SpySocket is stored by pointer inside the unique_ptr,
  // and the SpySocket itself lives on the stack for the test duration.
  std::unique_ptr<ISocket> makeUnique() {
    // Non-owning unique_ptr — the SpySocket is owned by the test.
    // Safe as long as the test outlives the session (always the case here).
    return std::unique_ptr<ISocket>(new SpySocketRef(*this));
  }

  // Accumulated bytes from all send() calls — header + payload together
  std::vector<std::uint8_t> sent;

  // Convenience: parse the MessageType from byte 5 of the first frame
  MessageType messageType() const {
    if (sent.size() < LPTF_HEADER_SIZE) return MessageType::END;
    return static_cast<MessageType>(sent[5]);
  }

  // Convenience: extract payload bytes (everything after the header)
  std::vector<std::uint8_t> payload() const {
    if (sent.size() <= LPTF_HEADER_SIZE) return {};
    return {sent.begin() + LPTF_HEADER_SIZE, sent.end()};
  }

  bool nothingSent() const { return sent.empty(); }

  // ── ISocket interface ─────────────────────────────────────────
  SocketResult send(const std::uint8_t* data, std::size_t len) override {
    sent.insert(sent.end(), data, data + len);
    return {static_cast<int>(len), SocketStatus::OK};
  }
  SocketResult recv(std::uint8_t*, std::size_t) override {
    return {0, SocketStatus::WOULD_BLOCK};
  }
  bool isValid() const override { return !closed_; }
  void close() override { closed_ = true; }
  bool connect(const std::string&, std::uint16_t) override { return false; }
  bool bind(std::uint16_t) override { return false; }
  bool listen(int) override { return false; }
  std::unique_ptr<ISocket> accept() override { return nullptr; }
  bool setNonBlocking(bool) override { return true; }
  int getFd() override { return -1; }
  std::string remoteAddress() const override { return ""; }
  std::uint16_t remotePort() const override { return 0; }

 private:
  bool closed_ = false;
  // Thin wrapper that delegates to the SpySocket without taking ownership.
  struct SpySocketRef : public ISocket {
    explicit SpySocketRef(SpySocket& spy) : spy_(spy) {}
    SocketResult send(const std::uint8_t* data, std::size_t length) override {
      return spy_.send(data, length);
    }
    SocketResult recv(std::uint8_t* data, std::size_t length) override {
      return spy_.recv(data, length);
    }
    bool isValid() const override { return spy_.isValid(); }
    void close() override { spy_.close(); }
    bool connect(const std::string& host, std::uint16_t port) override {
      return spy_.connect(host, port);
    }
    bool bind(std::uint16_t port) override { return spy_.bind(port); }
    bool listen(int backlog) override { return spy_.listen(backlog); }
    std::unique_ptr<ISocket> accept() override { return spy_.accept(); }
    bool setNonBlocking(bool on) override { return spy_.setNonBlocking(on); }
    int getFd() override { return spy_.getFd(); }
    std::string remoteAddress() const override { return spy_.remoteAddress(); }
    std::uint16_t remotePort() const override { return spy_.remotePort(); }
    SpySocket& spy_;
  };
};

// One-liner session factory — no pair, no reference_wrapper
inline AgentSession makeSession(SpySocket& spy) {
  return AgentSession(spy.makeUnique());
}

#endif
