#include "frame_transport.hpp"

#include <sstream>

#include "codec/protocol_helper.hpp"
#include "codec/protocol_serializer.hpp"
#include "exception/socket_exception.hpp"
#include "logger.hpp"
#include "socket/socket_factory.hpp"
#include "socket/tls_socket_factory.hpp"

// FrameTransport::FrameTransport() :  {}

std::optional<Frame> FrameTransport::tryExtractFrame() {
  if (!header_ && buffer_.size() >= LPTF_HEADER_SIZE) {
    header_ = ProtocolParser::parseHeader(slice(0, LPTF_HEADER_SIZE));
    consume(LPTF_HEADER_SIZE);
  }

  if (header_ && buffer_.size() >= header_->size) {
    Frame frame{*header_, slice(0, header_->size)};
    consume(header_->size);
    header_.reset();
    return frame;
  }

  return std::nullopt;
}

// ===== socket related methods =====
bool FrameTransport::isValid() const { return socket_ && socket_->isValid(); }

void FrameTransport::close() {
  if (socket_) {
    socket_->close();
  }
}

int FrameTransport::getFd() const { return socket_ ? socket_->getFd() : -1; }

SocketResult FrameTransport::send(const std::vector<std::uint8_t>& bytes) {
  return socket_->send(bytes);
}

// ===== Buffer related methods =====
void FrameTransport::consume(std::size_t n) {
  buffer_.erase(buffer_.begin(),
                buffer_.begin() + static_cast<std::ptrdiff_t>(n));
}

std::vector<std::uint8_t> FrameTransport::slice(std::size_t offset,
                                                std::size_t len) const {
  return {buffer_.begin() + static_cast<std::ptrdiff_t>(offset),
          buffer_.begin() + static_cast<std::ptrdiff_t>(offset + len)};
}

SocketResult FrameTransport::receiveIntoBuffer() {
  std::vector<std::uint8_t> temp(ProtocolHelper::kReceiveChunkSize);
  const SocketResult result = socket_->recv(temp.data(), temp.size());

  if (result.ok() && result.bytesTransferred > 0) {
    temp.resize(static_cast<std::size_t>(result.bytesTransferred));
    appendToBuffer(temp);
  }
  return result;
}

// Used for test only, create content inside buffer_
void FrameTransport::appendToBuffer(const std::vector<std::uint8_t>& bytes) {
  buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}

void FrameTransport::sendFrame(const Frame& frame) {
  // LOG
  std::ostringstream what;
  std::vector<std::uint8_t> frameBytes =
      ProtocolSerializer::serializeFrame(frame);
  what << " sending " << ProtocolHelper::messageTypeToString(frame.header.type)
       << " header+payload bytes=" << (LPTF_HEADER_SIZE + frame.payload.size());
  //      << " payload = ";
  // for (auto character : frame.payload) {
  //   what << character;
  // }

  Logger::info("frame transport", what.str());

  // Send frame
  const SocketResult result = send(frameBytes);

  if (!result.ok() ||
      static_cast<std::size_t>(result.bytesTransferred) != frameBytes.size()) {
    std::ostringstream what;
    what << "send header failed type="
         << ProtocolHelper::messageTypeToString(frame.header.type)
         << " sent=" << result.bytesTransferred
         << " expected=" << frameBytes.size()
         << " status=" << static_cast<int>(result.error);

    Logger::error("frame transport", what.str());
    throw SendFailure(ProtocolHelper::messageTypeToString(frame.header.type));
  }

  what.str("");
  what.clear();
  what << "end ok type="
       << ProtocolHelper::messageTypeToString(frame.header.type);
  Logger::info("frame transport", what.str());
}

void FrameTransport::onError(const std::vector<std::uint8_t>& payload) {
  const ErrorPayload error = ProtocolParser::parseErrorPayload(payload);
  std::ostringstream what;
  what << "code=" << static_cast<int>(error.code) << "  msg=" << error.message;
  Logger::error("frame transport", what.str());
  // std::cerr << "[← ERROR] code=" << static_cast<int>(error.code)
  //           << "  msg=" << error.message << "\n";
  //   running = false;
}

void FrameTransport::sendError(ErrorType code, const std::string& msg) {
  ErrorPayload error;
  error.code = code;
  error.message = msg;
  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeErrorPayload(error);
  Frame frame = {
      ProtocolHelper::createHeader(MessageType::INFERNO_ERROR, payload),
      payload};
  // sendRaw(agent, MessageType::INFERNO_ERROR, payload);
  sendFrame(frame);
}

