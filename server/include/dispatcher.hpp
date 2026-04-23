#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include <iostream>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "vector"
#include "client_session.hpp"
#include "protocol/protocol_serializer.hpp"

class Dispatcher {
 public:
  explicit Dispatcher();
  Dispatcher(const Dispatcher&) = delete;
  ~Dispatcher() = default;
  Dispatcher& operator=(const Dispatcher&) = delete;

  void dispatch(ClientSession& client, const Frame& frame);
  // ── Incoming message handlers ───────────────────────
  void onRegister(ClientSession& client, const std::vector<std::uint8_t>& payload);
  void onResponse(const std::vector<std::uint8_t>& payload);
  void onData(const std::vector<std::uint8_t>& payload);
  void onError(const std::vector<std::uint8_t>& payload);

  // ── Outgoing message builders ───────────────────────
  void sendCommand(CommandType type, const std::string& data = "");
  void sendError(ErrorType code, const std::string& msg);
  void sendDisconnect();

  // ── I/O ─────────────────────────────────────────────
    // std::vector<std::uint8_t> readExact(std::size_t n);
    void sendRaw(
        MessageType type,
        const std::vector<std::uint8_t>& payload = {}
    );

  std::uint16_t nextId();
};

#endif