#include "client_session.hpp"

  // One attempt to extract one complete frame from the buffer.
  // Returns the frame if complete, nullopt if more data is needed.
std::optional<Frame> ClientSession::tryExtractFrame() {
  if (!header && buffer.size() >= LPTF_HEADER_SIZE) {
    header = ProtocolParser::parseHeader(slice(0, LPTF_HEADER_SIZE));
    consume(LPTF_HEADER_SIZE);
  }

  if (header && buffer.size() >= header->size) {
    Frame frame{*header, slice(0, header->size)};
    consume(header->size);
    header.reset();
    return frame;
  }

  return std::nullopt;
}

void ClientSession::consume(std::size_t n) {
  buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(n));
}

std::vector<std::uint8_t> ClientSession::slice(std::size_t offset,
                                               std::size_t len) const {
  return {buffer.begin() + static_cast<std::ptrdiff_t>(offset),
          buffer.begin() + static_cast<std::ptrdiff_t>(offset + len)};
}