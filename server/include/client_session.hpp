#ifndef CLIENT_SESSION_HPP
#define CLIENT_SESSION_HPP

#include <memory>
#include <optional>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "socket/ISocket.hpp"

class ClientSession {
 public:
  explicit ClientSession(std::unique_ptr<ISocket> sock)
      : socket(std::move(sock)) {}

  ClientSession(const ClientSession&) = delete;
  ClientSession& operator=(const ClientSession&) = delete;
  ClientSession(ClientSession&&) = default;

  std::optional<Frame> tryExtractFrame();
  bool isValid() const { return socket && socket->isValid(); }
  std::unique_ptr<ISocket> socket;
  std::vector<std::uint8_t> buffer;
  const RegisterPayload& getClientInfo() const { return clientInfo; }
  void setClientInfo(const RegisterPayload& info) { clientInfo = info; }

  bool isRegistered() const { return isRegistered_; }
  void setRegistered(bool registered) { isRegistered_ = registered; }

 private:
  std::optional<LptfHeader> header;
  void consume(std::size_t n);
  std::vector<std::uint8_t> slice(std::size_t offset, std::size_t len) const;
  RegisterPayload clientInfo;
  bool isRegistered_ = false;
};

#endif