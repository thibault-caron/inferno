#include "dispatcher/dispatcher.hpp"

#include <sstream>

void Dispatcher::sendFrame(AgentSession& session, Frame& frame) {
  if (frame.payload.size() > MAX_VALUE_INT16) {
    logger_.error("payload too large");
    // std::cerr << "payload too large\n";
    throw InvalidSize("payload", std::to_string(frame.payload.size()));
  }

  // LOG
  std::ostringstream what;
  what << " sending " << SocketHelper::messageTypeToString(frame.header.type)
       << " header+payload bytes=" << (LPTF_HEADER_SIZE + frame.payload.size());
  logger_.info(what.str());
  // std::cout << "[" << senderName << "] sending "
  //           << SocketHelper::messageTypeToString(frame.header.type)
  //           << " header+payload bytes="
  //           << (LPTF_HEADER_SIZE + frame.payload.size()) << "\n";

  const std::vector<std::uint8_t> headerBytes =
      ProtocolSerializer::serializeHeader(frame.header);

  std::vector<uint8_t> frameBytes;
  frameBytes.reserve(headerBytes.size() + frame.payload.size());
  frameBytes.insert(frameBytes.end(), headerBytes.begin(), headerBytes.end());
  frameBytes.insert(frameBytes.end(), frame.payload.begin(),
                    frame.payload.end());
  // std::vector<std::uint8_t> frameBytes = headerBytes;
  // frameBytes.insert(headerBytes.end(), frame.payload.begin(),
  // frame.payload.end());

  // Send frame
  const SocketResult result = session.socket->send(frameBytes);

  if (!result.ok() ||
      static_cast<std::size_t>(result.bytesTransferred) != frameBytes.size()) {
    std::ostringstream what;
    what << "send header failed type="
         << SocketHelper::messageTypeToString(frame.header.type)
         << " sent=" << result.bytesTransferred
         << " expected=" << frameBytes.size()
         << " status=" << static_cast<int>(result.error);

    logger_.error(what.str());
    // std::cerr << "[agent] send header failed type="
    //           << SocketHelper::messageTypeToString(frame.header.type)
    //           << " sent=" << result.bytesTransferred
    //           << " expected=" << frameBytes.size()
    //           << " status=" << static_cast<int>(result.error) << "\n";
    // return false;
    throw SendFailure(SocketHelper::messageTypeToString(frame.header.type));
  }
  // std::ostringstream what;
  what.str("");
  what.clear();
  what << "end ok type="
       << SocketHelper::messageTypeToString(frame.header.type);
  logger_.info(what.str());
  // std::cout << "[" << senderName << "] send ok type="
  //           << SocketHelper::messageTypeToString(frame.header.type) << "\n";
}

void Dispatcher::onError(const std::vector<std::uint8_t>& payload) {
  const ErrorPayload error = ProtocolParser::parseErrorPayload(payload);
  std::ostringstream what;
  what << "code=" << static_cast<int>(error.code) << "  msg=" << error.message;
  logger_.error(what.str());
  // std::cerr << "[← ERROR] code=" << static_cast<int>(error.code)
  //           << "  msg=" << error.message << "\n";
  //   running = false;
}

void Dispatcher::sendError(AgentSession& agent, ErrorType code,
                           const std::string& msg) {
  ErrorPayload error;
  error.code = code;
  error.message = msg;
  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeErrorPayload(error);
  Frame frame = {SocketHelper::createHeader(MessageType::ERROR, payload),
                 payload};
  // sendRaw(agent, MessageType::ERROR, payload);
  sendFrame(agent, frame);
}