#ifndef FRAME_TRANSPORT_HPP
#define FRAME_TRANSPORT_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include "codec/protocol_parser.hpp"
#include "protocol/lptf_protocol.hpp"
#include "socket/i_socket.hpp"

class FrameTransport {
 public:
  explicit FrameTransport() {};
  explicit FrameTransport(std::unique_ptr<ISocket> sock)
      : socket_(std::move(sock)) {}
  FrameTransport(std::nullptr_t) = delete;

  FrameTransport(const FrameTransport&) = delete;
  FrameTransport& operator=(const FrameTransport&) = delete;
  FrameTransport(FrameTransport&&) = default;
  virtual ~FrameTransport() = default;

  std::optional<Frame> tryExtractFrame();
  // TODO : socket and buffer should be private and send() method too ?
  // ===== socket related methods =====
  bool isValid() const;
  void close();
  int getFd() const;
  void sendFrame(const Frame& frame);
  void onError(const std::vector<std::uint8_t>& payload);
  void sendError(ErrorType code, const std::string& msg);

  // ===== Register payload related methods =====
  virtual const OsInfoPayload& getAgentInfo() const = 0;
  virtual void setAgentInfo(const OsInfoPayload& info) = 0;

  // ===== Buffer related methods =====
  SocketResult receiveIntoBuffer();
  void appendToBuffer(const std::vector<std::uint8_t>& bytes);
  std::size_t bufferSize() const { return buffer_.size(); }

 protected:
  // ===== socket related methods =====
  SocketResult send(const std::vector<std::uint8_t>& bytes);
  std::unique_ptr<ISocket> socket_;
  std::vector<std::uint8_t> buffer_;
  std::optional<LptfHeader> header_;

  // ===== Buffer related private methods =====
  void consume(std::size_t n);
  std::vector<std::uint8_t> slice(std::size_t offset, std::size_t len) const;

  OsInfoPayload agentInfo_;
  //   Logger logger_{"agent"};
};

#endif