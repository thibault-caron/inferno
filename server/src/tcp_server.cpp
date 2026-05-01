#include "tcp_server.hpp"

#include <algorithm>
#include <iostream>

#include "protocol/protocol_parser.hpp"
#include "socket/SocketFactory.hpp"

TcpServer::TcpServer(const std::uint16_t port, const int backlog)
    : port_(port), backlog_(backlog) {}

bool TcpServer::start() {
  if (serverSocket_ && serverSocket_->isValid()) {
    std::cout << "[start] Socket is already valid on port " << port_ << '\n';
    return false;
  }
  serverSocket_ = SocketFactory::createTCP();
  if (!serverSocket_ || !serverSocket_->isValid()) {
    std::cerr << "[start] Failed to create server socket on port " << port_ << '\n';
    return false;
  }
  if (!serverSocket_->bind(port_)) {
    std::cerr << "[start] Failed to bind server socket on port " << port_ << '\n';
    return false;
  }
  std::cout << "[start] Server try to listen\n";
  return serverSocket_->listen(backlog_);
}

std::unique_ptr<ISocket> TcpServer::acceptAgent() const {
  if (!serverSocket_) {
    return nullptr;
  }
  return serverSocket_->accept();
}

// SocketResult TcpServer::receiveFromAgent(std::unique_ptr<ISocket>& agent) {
//   std::vector<std::uint8_t> tmp(2048, 0);
//   const SocketResult recvResult = agent->recv(tmp.data(), tmp.size());
//   if (recvResult.ok() && recvResult.bytesTransferred > 0) {
//     appendToReceiveBuffer(
//         tmp.data(), static_cast<std::size_t>(recvResult.bytesTransferred));
//   }
//   return recvResult;
// }

// SocketResult TcpServer::sendToAgent(
//     ISocket& agent, const std::vector<std::uint8_t>& payload) const {
//   return agent.send(payload);
// }

// void TcpServer::appendToReceiveBuffer(const std::uint8_t* data,
//                                       const std::size_t len) {
//   receiveBuffer_.insert(receiveBuffer_.end(), data, data + len);
// }

// std::size_t TcpServer::parseAvailableFrames() {
//   std::size_t parsedCount = 0;

//   // while(receiveBuffer_.size() < LPTF_HEADER_SIZE ) {

//   // }
//   while (receiveBuffer_.size() >= LPTF_HEADER_SIZE) {
//     LptfHeader header;
//     try {
//       header = ProtocolParser::parseHeader(
//           slice(receiveBuffer_, 0, LPTF_HEADER_SIZE));
//     } catch (const std::exception&) {
//       receiveBuffer_.clear();
//       break;
//     }

//     const std::size_t frameSize = LPTF_HEADER_SIZE + header.size;
//     // if (receiveBuffer_.size() < frameSize) {
//     //   break;
//     // }
//     if (receiveBuffer_.size() >= frameSize ) {
//       ParsedFrame frame{header, slice(receiveBuffer_, LPTF_HEADER_SIZE,
//                                       static_cast<std::size_t>(header.size))};
//       parsedFrames_.push_back(frame);
//       printParsedPayload(frame);

//       receiveBuffer_.erase(
//           receiveBuffer_.begin(),
//           receiveBuffer_.begin() + static_cast<std::ptrdiff_t>(frameSize));
//       ++parsedCount;
//     }
//   }
//   return parsedCount;
// }

// const std::vector<std::uint8_t>& TcpServer::receiveBuffer() const {
//   return receiveBuffer_;
// }

// const std::vector<TcpServer::ParsedFrame>& TcpServer::parsedFrames() const {
//   return parsedFrames_;
// }

// void TcpServer::clearParsedFrames() { parsedFrames_.clear(); }

// std::vector<std::uint8_t> TcpServer::slice(const std::vector<std::uint8_t>&
// src,
//                                            const std::size_t offset,
//                                            const std::size_t len) {
//   const std::vector<std::uint8_t>::const_iterator beginIt = src.begin() +
//   static_cast<std::ptrdiff_t>(offset); const
//   std::vector<std::uint8_t>::const_iterator endIt = beginIt +
//   static_cast<std::ptrdiff_t>(len); return std::vector<std::uint8_t>(beginIt,
//   endIt);
// }

// TODO : put inside shared library with every other toMessageType ...etc
// std::string TcpServer::messageTypeToString(const MessageType type) {
//   switch (type) {
//     case MessageType::REGISTER:
//       return "REGISTER";
//     case MessageType::DATA:
//       return "DATA";
//     case MessageType::COMMAND:
//       return "COMMAND";
//     case MessageType::RESPONSE:
//       return "RESPONSE";
//     case MessageType::DISCONNECT:
//       return "DISCONNECT";
//     case MessageType::ERROR:
//       return "ERROR";
//     default:
//       return "UNKNOWN";
//   }
// }

// void TcpServer::printParsedPayload(const ParsedFrame& frame) {
//   std::cout << "Parsed type=" << messageTypeToString(frame.header.type)
//             << " payload_size=" << frame.payload.size();

//   if (frame.header.type == MessageType::REGISTER) {
//     const RegisterPayload payload =
//         ProtocolParser::parseRegisterPayload(frame.payload);
//     std::cout << " hostname=" << payload.hostname;
//   }

//   std::cout << '\n';
// }

// void TcpServer::run() {
//   std::unique_ptr<ISocket> agent = acceptAgent();
//   bool run = true;
//   while (run) {

//   }

//   const SocketResult result = receiveFromAgent(agent);
//   std::size_t parsedCount = parseAvailableFrames();

// }