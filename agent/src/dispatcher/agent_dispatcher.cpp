#include "dispatcher/agent_dispatcher.hpp"

#include <sstream>

#include "codec/protocol_helper.hpp"
#include "codec/protocol_parser.hpp"
#include "codec/protocol_serializer.hpp"
#include "exception/lptf_exception.hpp"
#include "logger.hpp"
#include "socket/i_socket.hpp"

AgentDispatcher::AgentDispatcher(ISystemMonitor& monitor)
    : IAgentDispatcher(), monitor_(monitor) {}

void AgentDispatcher::handleFrame(FrameTransport& agent, const Frame& frame) {
  AgentSession& session = static_cast<AgentSession&>(agent);
  switch (frame.header.type) {
    case MessageType::COMMAND:
      return onCommand(session, frame.payload);
    case MessageType::DISCONNECT:
      return onDisconnect(session);
    case MessageType::INFERNO_ERROR: {
      return onError(frame.payload);
    }
    default:
      return agent.sendError(ErrorType::UNKNOWN_TYPE,
                             "Unexpected received message type for agent");
  }
}

void AgentDispatcher::startMetrics(AgentSession& session,
                                   const CommandPayload& command) {
  Logger::info("agent dispatcher", "received COMMAND START_METRICS id=" +
                                       std::to_string(command.id));
  metricsController_->start(session);
  return send(session, command.id, ResponseStatus::OK, {},
              CommandType::START_METRICS);
}

void AgentDispatcher::stopMetrics(AgentSession& session,
                                  const CommandPayload& command) {
  Logger::info("agent dispatcher", "received COMMAND STOP_METRICS id=" +
                                       std::to_string(command.id));
  metricsController_->stop();
  return send(session, command.id, ResponseStatus::OK, {},
              CommandType::STOP_METRICS);
}

void AgentDispatcher::osInfo(AgentSession& session,
                             const CommandPayload& command) {
  std::ostringstream what;
  what << "received COMMAND OS_INFO id=" << command.id;
  Logger::info("agent dispatcher", what.str());

  OsInfoPayload payload = monitor_.getOsInfo();

  const std::vector<std::uint8_t> OsInfoPayload =
      ProtocolSerializer::serializeOsInfoPayload(payload);

  send(session, command.id, ResponseStatus::OK, OsInfoPayload,
       CommandType::OS_INFO);
}

void AgentDispatcher::processesList(AgentSession& session,
                                    const CommandPayload& command) {
  std::ostringstream what;
  what << "received COMMAND RUNNING_PROCESSES id=" << command.id;
  Logger::info("agent dispatcher", what.str());

  const std::vector<ProcessInfo> processes = monitor_.getProcessList();
  const std::vector<std::uint8_t> processBytes =
      ProtocolSerializer::serializeProcessInfoList(processes);

  return send(session, command.id, ResponseStatus::OK, processBytes,
              CommandType::RUNNING_PROCESSES);
}

void AgentDispatcher::shellCommand(AgentSession& session,
                                   const CommandPayload& command) {
  std::ostringstream what;
  what << "received COMMAND SHELL id=" << command.id;
  Logger::info("agent dispatcher", what.str());

  const std::string output = monitor_.executeShell(command.data);

  return send(session, command.id, ResponseStatus::OK,
              {output.begin(), output.end()}, CommandType::SHELL);
}

void AgentDispatcher::onError(const std::vector<std::uint8_t>& payload) {
  std::ostringstream what;
  try {
    const ErrorPayload errorPayload =
        ProtocolParser::parseErrorPayload(payload);
    what << "server ERROR code=" << static_cast<int>(errorPayload.code)
         << " message=" << errorPayload.message;
    Logger::error("agent dispatcher", what.str());
  } catch (...) {
    Logger::error("agent dispatcher", "received malformed ERROR payload");
  }
}
void AgentDispatcher::onDisconnect(AgentSession& session) {
  Logger::info("agent dispatcher", "received DISCONNECT");
  if (metricsController_.get()->isActive()) {
    CommandPayload cmd;
    cmd.type = CommandType::STOP_METRICS;
    stopMetrics(session, cmd);
  }
  session.setDisconnectRequest(true);
  // session.close();  // check for valid socket already inside close method
  // session.socket.reset();
  // connected_ = false;
}

void AgentDispatcher::onCommand(AgentSession& session,
                                const std::vector<std::uint8_t>& payload) {
  try {
    const CommandPayload cmd = ProtocolParser::parseCommandPayload(payload);

    switch (cmd.type) {
      case CommandType::OS_INFO:
        return osInfo(session, cmd);
      case CommandType::RUNNING_PROCESSES:
        return processesList(session, cmd);
      case CommandType::SHELL:
        return shellCommand(session, cmd);
      case CommandType::START_METRICS:
        return startMetrics(session, cmd);
      case CommandType::STOP_METRICS:
        return stopMetrics(session, cmd);
      default:
        return session.sendError(ErrorType::UNKNOWN_COMMAND,
                                 "Command not implemented in minimal agent");
    }

  } catch (const std::exception& ex) {
    std::ostringstream what;
    what << "invalid COMMAND payload: " << ex.what();
    Logger::error("agent dispatcher", what.str());
    return session.sendError(ErrorType::INVALID_FORMAT,
                             "Invalid COMMAND payload");
  }
}

void AgentDispatcher::sendRegister(AgentSession& session) {
  std::ostringstream what;
  CommandPayload command;
  command.id = 0;
  command.type = CommandType::OS_INFO;
  // OsInfoPayload payload
  if (
      // session.getRegistered_() == RegisterState::PENDING &&
      session.getAgentInfo() == OsInfoPayload{}) {
    session.setAgentInfo(monitor_.getOsInfo());
    // payload = monitor_.getOsInfo();
  }
  // session.setAgentInfo(payload);
  session.setRegistered_(RegisterState::SENT);

  Logger::info("agent dispatcher", what.str());
  send(session, command.id, ResponseStatus::OK,
       ProtocolSerializer::serializeOsInfoPayload(session.getAgentInfo()),
       CommandType::OS_INFO, MessageType::REGISTER);
  // HERE !
  // send(session, command.id, ResponseStatus::OK,
  //      ProtocolSerializer::serializeOsInfoPayload(session.getAgentInfo()),
  //      MessageType::DASHBOARD_REGISTER);
}

void AgentDispatcher::setMetricsController(
    std::shared_ptr<MetricsController> controller) {
  metricsController_ = controller;
}

// GROS FDP

void AgentDispatcher::send(AgentSession& session, std::uint16_t id,
                           ResponseStatus status,
                           const std::vector<std::uint8_t>& data,
                           CommandType cmdType, MessageType type) {
  switch (type) {
    case MessageType::RESPONSE: {
      sendResponseChunked(session, id, status, cmdType, data);
      return;
    }
    case MessageType::REGISTER: {
      Frame frame = {ProtocolHelper::createHeader(type, data), data};
      session.sendFrame(frame);
      return;
    }
    // DEBUG ONLY
    // case MessageType::DASHBOARD_REGISTER: {
    //   Frame frame = {
    //       ProtocolHelper::createHeader(MessageType::DASHBOARD_REGISTER,
    //       data), data};
    //   session.sendFrame(frame);
    //   return;
    // }
    default: {
      Logger::info("agent dispatcher",
                   std::string("unexpected message type : ") +
                       ProtocolHelper::messageTypeToString(type));
    }
  }
}

void AgentDispatcher::sendResponseChunked(
    AgentSession& session, std::uint16_t id, ResponseStatus status,
    CommandType cmdType, const std::vector<std::uint8_t>& data) {
  const std::size_t maxChunkSize =
      KMAX_U16_VALUE - RESPONSE_FIXED_BYTES - LPTF_HEADER_SIZE;

  // Calculate how many chunks needed
  std::size_t totalChunks = (data.size() + maxChunkSize - 1) / maxChunkSize;
  if (totalChunks == 0) totalChunks = 1;  // At least 1 chunk
  if (totalChunks > std::numeric_limits<std::uint8_t>::max()) {
    throw InvalidFieldValue(
        "total_chunks", std::to_string(static_cast<std::uint8_t>(totalChunks)));
  }

  // Send each chunk
  for (std::size_t chunkIdx = 0; chunkIdx < totalChunks; ++chunkIdx) {
    std::size_t start = chunkIdx * maxChunkSize;
    std::size_t end = std::min(start + maxChunkSize, data.size());

    std::vector<uint8_t> responsePayload = createResponseChunk(
        id, status, cmdType, chunkIdx, totalChunks, start, end, data);

    Frame frame = {
        ProtocolHelper::createHeader(MessageType::RESPONSE, responsePayload),
        responsePayload};

    session.sendFrame(frame);
  }
}

std::vector<std::uint8_t> AgentDispatcher::createResponseChunk(
    std::uint16_t id, ResponseStatus status, CommandType cmdType,
    std::size_t chunkIndex, std::size_t totalChunks, std::size_t start,
    std::size_t end, const std::vector<std::uint8_t>& data) {
  ResponsePayload response;
  response.id = id;
  response.status = status;
  response.type = cmdType;
  response.total_chunks = static_cast<std::uint8_t>(totalChunks);
  response.chunk_index = static_cast<std::uint8_t>(chunkIndex);
  response.data = {data.begin() + start, data.begin() + end};

  return ProtocolSerializer::serializeResponsePayload(response);
}