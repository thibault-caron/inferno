#include "tcp_server.hpp"

// #include <string>

// #include <algorithm>
#include <iostream>
#include <sstream>

// #include "protocol/protocol_parser.hpp"
#include "socket/socket_factory.hpp"
// #include "protocol/lptf_protocol.hpp"

TcpServer::TcpServer(const std::uint16_t port, const int backlog)
    : port_(port), backlog_(backlog) {}

bool TcpServer::start() {
  if (serverSocket_ && serverSocket_->isValid()) {
    std::ostringstream what;
    what << "[start] Socket is already valid on port " << port_ ;
    logger_.error(what.str());
    // std::cout << "[start] Socket is already valid on port " << port_ << '\n';
    return false;
  }
  serverSocket_ = SocketFactory::createTCP();
  if (!serverSocket_ || !serverSocket_->isValid()) {
    std::ostringstream what;
    what << "[start] Failed to create server socket on port " << port_;
    // std::cerr << "[start] Failed to create server socket on port " << port_
    // << '\n';
    logger_.error(what.str());
    return false;
  }
  if (!serverSocket_->bind(port_)) {
    std::ostringstream what;
    what << "[start] Failed to bind server socket on port " << port_;
    logger_.error(what.str());
    // std::cerr << "[start] Failed to bind server socket on port " << port_ <<
    // '\n';
    return false;
  }
  logger_.info("[start] Server try to listen");
  // std::cout << "[start] Server try to listen\n";
  return serverSocket_->listen(backlog_);
}

std::unique_ptr<ISocket> TcpServer::acceptAgent() const {
  if (!serverSocket_) {
    return nullptr;
  }
  return serverSocket_->accept();
}