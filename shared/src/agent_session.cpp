#include "agent_session.hpp"

#include "protocol/protocol_helper.hpp"

std::optional<Frame> AgentSession::tryExtractFrame() {
  if (!header_ && buffer.size() >= LPTF_HEADER_SIZE) {
    header_ = ProtocolParser::parseHeader(slice(0, LPTF_HEADER_SIZE));
    consume(LPTF_HEADER_SIZE);
  }

  if (header_ && buffer.size() >= header_->size) {
    Frame frame{*header_, slice(0, header_->size)};
    consume(header_->size);
    header_.reset();
    return frame;
  }

  return std::nullopt;
}

void AgentSession::consume(std::size_t n) {
  buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(n));
}

std::vector<std::uint8_t> AgentSession::slice(std::size_t offset,
                                              std::size_t len) const {
  return {buffer.begin() + static_cast<std::ptrdiff_t>(offset),
          buffer.begin() + static_cast<std::ptrdiff_t>(offset + len)};
}

SocketResult AgentSession::receiveIntoBuffer() {
  std::vector<std::uint8_t> temp(ProtocolHelper::kReceiveChunkSize);
  const SocketResult result = socket->recv(temp.data(), temp.size());

  if (result.ok() && result.bytesTransferred > 0) {
    temp.resize(static_cast<std::size_t>(result.bytesTransferred));
    buffer.insert(buffer.end(), temp.begin(), temp.end());
    // std::cout << "[agent] recv bytes=" << result.bytesTransferred
    //           << " buffer_size=" << session.buffer.size() << "\n";
  }
  // else {
  //   std::cout << "[agent] recv status=" << static_cast<int>(result.error)
  //             << " bytes=" << result.bytesTransferred << "\n";
  // }
  return result;
};

void AgentSession::resetSession() {
  isRegistered_ = false;
  registered_ = RegisterState::PENDING;
  buffer.clear();
  header_.reset();
  socket = SocketFactory::createTCP(); 
}