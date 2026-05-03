#ifndef ENV_HELPER_HPP
#define ENV_HELPER_HPP

#include <iostream>
#include <string>

#include "agent_session.hpp"
#include "socket/ISocket.hpp"
#include "socket/socket_factory.hpp"
class EnvHelper {
 public:
  EnvHelper() = delete;
  EnvHelper(const EnvHelper&) = delete;
  EnvHelper& operator=(const EnvHelper&) = delete;

  static std::uint16_t resolveServerPort();

  static std::string resolveServerHost();

  static const std::uint16_t SERVER_PORT = 8888;

 private:
};

#endif