#include "env_helper.hpp"

std::uint16_t EnvHelper::resolveServerPort() {
  const char* portEnv = std::getenv("SERVER_PORT");
  if (!portEnv) {
    std::cout << "SERVER_PORT not found in .env" << SERVER_PORT << '\n';
    return SERVER_PORT;
  }

  try {
    const int parsed = std::stoi(portEnv);
    if (parsed > 0 && parsed <= 65535) {
      std::cout << "Resolved server port from environment: " << parsed << '\n';
      return static_cast<std::uint16_t>(parsed);
    }
  } catch (...) {
  }
  std::cout << "Invalid SERVER_PORT value in .env: " << portEnv
            << ", using default " << SERVER_PORT << '\n';
  return SERVER_PORT;
}

std::string EnvHelper::resolveServerHost() {
  const char* hostEnv = std::getenv("SERVER_HOST");
  if (hostEnv && hostEnv[0] != '\0') {
    std::cout << "Resolved server host from environment: " << hostEnv << '\n';
    return std::string(hostEnv);
  }
  return "server";
}