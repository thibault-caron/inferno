#include "agent_session.hpp"

#include "protocol/protocol_helper.hpp"
#include "socket/socket_factory.hpp"

std::optional<Frame> AgentSession::tryExtractFrame() {
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

void AgentSession::resetSession() {
  isRegistered_ = false;
  registered_ = RegisterState::PENDING;
  buffer_.clear();
  header_.reset();
  socket_ = SocketFactory::createTCP();
}

// ===== socket related methods =====
bool AgentSession::isValid() const { return socket_ && socket_->isValid(); }

void AgentSession::close() {
  if (socket_) {
    socket_->close();
  }
}

bool AgentSession::connect(const std::string& host, std::uint16_t port) {
    return socket_ && socket_->connect(host, port);
}

int AgentSession::getFd() const {
    return socket_ ? socket_->getFd() : -1;
}

SocketResult AgentSession::send(const std::vector<std::uint8_t>& bytes) {
  return socket_->send(bytes);
}

// ===== Register payload related methods =====
const RegisterPayload& AgentSession::getAgentInfo() const {
  if (!isRegistered_) {
    throw std::runtime_error("Agent not registered");
  }
  return agentInfo_;
}

void AgentSession::setAgentInfo(const RegisterPayload& info) {
  agentInfo_ = info;
  isRegistered_ = true;
}

// ===== Buffer related methods =====
void AgentSession::consume(std::size_t n) {
  buffer_.erase(buffer_.begin(),
                buffer_.begin() + static_cast<std::ptrdiff_t>(n));
}

std::vector<std::uint8_t> AgentSession::slice(std::size_t offset,
                                              std::size_t len) const {
  return {buffer_.begin() + static_cast<std::ptrdiff_t>(offset),
          buffer_.begin() + static_cast<std::ptrdiff_t>(offset + len)};
}

SocketResult AgentSession::receiveIntoBuffer() {
  std::vector<std::uint8_t> temp(ProtocolHelper::kReceiveChunkSize);
  const SocketResult result = socket_->recv(temp.data(), temp.size());

  if (result.ok() && result.bytesTransferred > 0) {
    temp.resize(static_cast<std::size_t>(result.bytesTransferred));
    appendToBuffer(temp);
  }
  return result;
}

// Used for test only, create content inside buffer_
void AgentSession::appendToBuffer(const std::vector<std::uint8_t>& bytes) {
  buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}