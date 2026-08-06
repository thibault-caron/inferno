#ifndef SOCKET_FACTORY_HPP
#define SOCKET_FACTORY_HPP
#include "i_socket.hpp"
#include <memory>

#ifdef _WIN32
  #include "socket/windows_socket.hpp"
#elif defined(__linux__)
  #include "socket/linux_socket.hpp"
#elif defined(__APPLE__)
  #include "socket/linux_socket.hpp" // macOS is POSIX-compatible, same impl works
#endif

class SocketFactory {
public:
    static std::unique_ptr<ISocket> createTCP();
};

#endif