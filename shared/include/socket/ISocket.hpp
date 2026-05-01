#ifndef ISOCKET
#define ISOCKET
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "protocol/lptf_protocol.hpp"

constexpr std::uint16_t SERVER_PORT = 8888;

enum class SocketStatus {
  OK,
  CONNECTION_REFUSED,
  CONNECTION_RESET,
  TIMED_OUT,
  WOULD_BLOCK,  // for non-blocking I/O
  ADDRESS_IN_USE,
  INVALID_ARGUMENT,
  NETWORK_UNREACHABLE,
  UNKNOWN,
};

struct SocketResult {
  int bytesTransferred = 0;  // -1 on error
  SocketStatus error = SocketStatus::OK;

  bool ok() const { return error == SocketStatus::OK; }
  bool wouldBlock() const { return error == SocketStatus::WOULD_BLOCK; }
};

class ISocket {
 public:
  virtual ~ISocket() = default;

  // Agent side
  virtual bool connect(const std::string& host, std::uint16_t port) = 0;

  // Server side
  virtual bool bind(std::uint16_t port) = 0;
  virtual bool listen(
      int backlog = 10) = 0;  // virtual pure AVEC argument par défaut
  // Returns nullptr on failure
  virtual std::unique_ptr<ISocket> accept() = 0;

  // I/O — works with your binary protocol buffers directly
  virtual SocketResult send(const std::uint8_t* data, std::size_t len) = 0;
  virtual SocketResult recv(std::uint8_t* data, std::size_t len) = 0;
  virtual int getFd() = 0;

  // Convenience overloads for vectors
  SocketResult send(const std::vector<std::uint8_t>& buf) {
    return send(buf.data(), buf.size());
  }
  SocketResult recv(std::vector<std::uint8_t>& buf) {
    return recv(buf.data(), buf.size());
  }

  virtual void close() = 0;
  virtual bool setNonBlocking(bool on) = 0;
  virtual bool isValid() const = 0;

  // For debugging / logging
  virtual std::string remoteAddress() const = 0;
  virtual std::uint16_t remotePort() const = 0;
};

#endif