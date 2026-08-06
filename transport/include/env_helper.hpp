#ifndef ENV_HELPER_HPP
#define ENV_HELPER_HPP

#include <iostream>
#include <string>

#include "socket/i_socket.hpp"
#include "socket/socket_factory.hpp"

namespace EnvHelper {
std::uint16_t resolvePort(const std::string& portName = "SERVER_PORT");
std::string resolveString(const std::string& variableName);
// std::string resolveServerHost();

bool resolveTlsEnabled();
void loadEnvFile(const std::string& filePath = ".env");
std::string getFromCache(const std::string& key);

}  // namespace EnvHelper
// class EnvHelper {
//  public:

//  private:
// };

#endif