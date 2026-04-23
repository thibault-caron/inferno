#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "socket/ISocket.hpp"

class TcpServer {
 public:
  // struct ParsedFrame {
  //   LptfHeader header;
  //   std::vector<std::uint8_t> payload;
  // };

  // explicit TcpServer(std::uint16_t port, int backlog = 10);
  explicit TcpServer(std::uint16_t port, int backlog = 10);
  TcpServer(const TcpServer&) = delete;
  TcpServer& operator=(const TcpServer&) = delete;
  ~TcpServer() = default;

  bool start();
  std::unique_ptr<ISocket> acceptClient() const;

  // SocketResult receiveFromClient(std::unique_ptr<ISocket>& client);
  // SocketResult sendToClient(ISocket& client,
  //                           const std::vector<std::uint8_t>& payload) const;

  // void appendToReceiveBuffer(const std::uint8_t* data, std::size_t len);
  // std::size_t parseAvailableFrames();

  // const std::vector<std::uint8_t>& receiveBuffer() const;
  // const std::vector<ParsedFrame>& parsedFrames() const;
  // void clearParsedFrames();
  // void run();

 private:
  // static std::vector<std::uint8_t> slice(const std::vector<std::uint8_t>& src,
  //                                        std::size_t offset, std::size_t len);
  // static std::string messageTypeToString(MessageType type);
  // static void printParsedPayload(const ParsedFrame& frame);

  std::uint16_t port_;
  int backlog_;
  std::unique_ptr<ISocket> serverSocket_;
  std::vector<std::uint8_t> receiveBuffer_;
  // std::vector<ParsedFrame> parsedFrames_;
};

#endif