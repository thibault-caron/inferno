#ifndef SOCKET_FACTORY
#define SOCKET_FACTORY
#include "ISocket.hpp"
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