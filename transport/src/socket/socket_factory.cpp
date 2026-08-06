// SocketFactory.cpp
#include "socket/socket_factory.hpp"


std::unique_ptr<ISocket> SocketFactory::createTCP() {
#ifdef _WIN32
    return std::make_unique<WindowsSocket>();
#else
    return std::make_unique<LinuxSocket>();
#endif
}