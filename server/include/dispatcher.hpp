#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "client_session.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/ISocket.hpp"

class Dispatcher {
 public:
  // explicit Dispatcher(std::unique_ptr<ISocket>& socket);
  explicit Dispatcher();
  Dispatcher(const Dispatcher&) = delete;
  ~Dispatcher() = default;
  Dispatcher& operator=(const Dispatcher&) = delete;

  void dispatch(ClientSession& client, const Frame& frame);
  // ── Incoming message handlers ───────────────────────
  void onRegister(ClientSession& client,
                  const std::vector<std::uint8_t>& payload);
  void onResponse(ClientSession& client, const std::vector<std::uint8_t>& payload);
  void onData(const std::vector<std::uint8_t>& payload);
  void onError(const std::vector<std::uint8_t>& payload);

  // ── Outgoing message builders ───────────────────────
  void sendCommand(ClientSession& client, CommandType type, const std::string& data = "");
  void sendError(ClientSession& client, ErrorType code, const std::string& msg);
  void sendDisconnect(ClientSession& client);

  // ── I/O ─────────────────────────────────────────────
  void sendRaw(ClientSession& client, MessageType type,
               const std::vector<std::uint8_t>& payload = {});

  std::uint16_t nextId();

 private:
  // ISocket& socket_;
  std::uint16_t nextCmdId = 0;
};

#endif