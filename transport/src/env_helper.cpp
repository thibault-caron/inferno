#include "env_helper.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <unordered_map>
#include <iostream>

namespace EnvHelper {

std::unordered_map<std::string, std::string> envCache;
bool cacheLoaded = false;
std::vector<std::string> deFaultEnvPaths = {".env", "../.env", "../../.env"};

std::uint16_t resolvePort(const std::string& portName) {
  const char* portEnv = std::getenv(portName.c_str());

  // Fallback to cache if not in process environment
  std::string portStr;
  if (portEnv) {
    portStr = portEnv;
  } else {
    portStr = getFromCache(portName);  // Check .env file
  }

  std::cout << "DEBUG: portName=" << portName << " portStr=" << portStr
            << " SERVER_PORT=" << SERVER_PORT << '\n';

  if (portStr.empty()) {
    return SERVER_PORT;  // Default
  }

  try {
    int parsed = std::stoi(portStr);
    if (parsed > 0 && parsed <= 65535) {
      return static_cast<std::uint16_t>(parsed);
    }
  } catch (...) {
  }

  return SERVER_PORT;
}

std::string resolveString(const std::string& variableName) {
  const char* envVariable = std::getenv(variableName.c_str());
  if (envVariable && envVariable[0] != '\0') {
    std::cout << "Resolved " << variableName << " from environment.\n";
    return std::string(envVariable);
  }

  std::string cachedValue = getFromCache(variableName);
  if (!cachedValue.empty()) {
    std::cout << "Resolved " << variableName << " from .env file.\n";
    return cachedValue;
  }

  if (variableName == "DASHBOARD_SERVER_HOST" || variableName == "SERVER_HOST")
    return "localhost";

  return "";
}

bool resolveTlsEnabled() {
  const char* tlsEnv = std::getenv("TLS");
  std::string value;

  if (tlsEnv) {
    value = tlsEnv;
  } else {
    value = getFromCache("TLS");  // Check cache
  }

  if (value.empty()) {
    return true;  // default
  }

  std::transform(value.begin(), value.end(), value.begin(), ::tolower);
  // return value == "true" || value == "1" || value == "yes" || value == "on";
  return value != "false" && value != "0" && value != "no" && value != "off";
}

void loadEnvFile(const std::string& filePath) {
  if (cacheLoaded) return;  // Only parse once

  std::vector<std::string> paths = {filePath};
  paths.insert(paths.end(), deFaultEnvPaths.begin(), deFaultEnvPaths.end());

  for (const auto& path : paths) {
    std::ifstream file(path);
    if (!file.is_open()) {
      std::cout << ".env file not found at " << path << '\n';
      // cacheLoaded = true;
      // return;
      continue;
    }

    std::string line;
    while (std::getline(file, line)) {
      // Skip empty lines and comments
      if (line.empty() || line[0] == '#') continue;

      // Find the '=' separator
      size_t pos = line.find('=');
      if (pos == std::string::npos) continue;

      // Extract key and value, trim whitespace if needed
      std::string key = line.substr(0, pos);
      std::string value = line.substr(pos + 1);

      // Optional: trim whitespace around key and value
      // key.erase(remove(key.begin(), key.end(), ' '), key.end());

      envCache[key] = value;
    }

    cacheLoaded = true;
  }
  if (envCache.find("SERVER_HOST") == envCache.end()) {
    envCache["SERVER_HOST"] = "server";  // Container default
  }
  return;
}

std::string getFromCache(const std::string& key) {
  loadEnvFile("../../.env");
  auto it = envCache.find(key);
  return (it != envCache.end()) ? it->second : "";
}

}  // namespace EnvHelper