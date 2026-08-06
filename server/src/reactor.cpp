#include "reactor.hpp"

#include <sstream>

void Reactor::run() {
  if (!poller_.add(server_.getFd(),
                   WatchFlags::READ | WatchFlags::INFERNO_ERROR)) {
    Logger::error("reactor", "failed to add server fd");
    return;
  }
  // poller_.add(server_.getFd(), WatchFlags::READ | WatchFlags::INFERNO_ERROR);
  running_ = true;

  std::vector<ReadyEvent> events;

  while (running_) {
    const int firedCount = poller_.wait(events, -1);
    // const int firedCount = poller_.wait(events, 1000); // for debug, waits
    // for 1second
    if (firedCount <= 0) {
      running_ = false;
      Logger::info("reactor", "will stop since fire count is <= 0");
    } else {
      for (const ReadyEvent& event : events) {
        if (event.fileDescriptor == server_.getFd()) {
          onNewConnection();
        } else if (event.readable) {
          Logger::info("reactor", "has new event to handle");
          onAgentReady(event.fileDescriptor);
        } else if (event.error) {
          Logger::info("reactor", "has error");
          onAgentDisconnected(event.fileDescriptor);
        }
      }
    }
  }
}

void Reactor::onNewConnection() {
  std::unique_ptr<ISocket> incoming = server_.acceptAgent();

  if (!incoming) return;

  std::ostringstream what;
  const int fd = incoming->getFd();
  if (!poller_.add(fd, WatchFlags::READ | WatchFlags::INFERNO_ERROR)) {
    Logger::error("reactor", "Failed to watch fd " + std::to_string(fd));
    return;
  }
  what << "Agent connected: " << incoming->remoteAddress() << ':'
       << incoming->remotePort();
  // agents_.emplace(fd, AgentConnection(std::move(incoming)));
  sessionManager_.addAgent(fd, std::move(incoming));
  Logger::info("reactor", what.str());
}

void Reactor::onAgentReady(int fileDescriptor) {
  // AgentConnection& session = agents_.at(fileDescriptor);
  Logger::info("reactor", "on agent ready begining");
  AgentConnection& session = sessionManager_.getAgent(fileDescriptor);
  const SocketResult result = session.receiveIntoBuffer();

  Logger::info("reactor", "on agent ready before if socket ok");

  if (!result.ok() || result.error == SocketStatus::CONNECTION_RESET ||
      result.bytesTransferred <= 0) {
    std::ostringstream what;
    what << "Agent " << fileDescriptor << " disconnected\n";
    Logger::info("reactor", what.str());
    // std::cout << "[server] Agent " << fileDescriptor << " disconnected\n";
    onAgentDisconnected(fileDescriptor);
    return;
  }

  Logger::info("reactor", "on agent ready before while loop");

  while (std::optional<Frame> frame = session.tryExtractFrame()) {
    bool canHandleFrame = !session.getIsRegistered() &&
                          frame->header.type != MessageType::REGISTER &&
                          frame->header.type != MessageType::DASHBOARD_REGISTER;

    Logger::info(
        "reactor",
        std::string("on agent ready before if in while loop, bool value:") +
            std::string(canHandleFrame ? "true" : "false"));
    if (canHandleFrame) {
      if (sessionManager_.isDashboard()) {
        sessionManager_.getDashboard().sendError(
            ErrorType::INVALID_FORMAT, "First message must be REGISTER");
      }
    } else {
      dispatcher_.handleFrame(session, frame.value());
    }
  }
  Logger::info("reactor", "who ? " + session.getAgentInfo().hostname);
  Logger::info("reactor", "on agent ready end");
}

void Reactor::onAgentDisconnected(int fileDescriptor) {
  Logger::info("reactor", "disconnection clean up");
  poller_.remove(fileDescriptor);
  if (fileDescriptor == sessionManager_.getDashboardFd()) {
    sessionManager_.resetDashboard();
  } else {
    dispatcher_.onAgentDisconnect(sessionManager_.getAgent(fileDescriptor));
    sessionManager_.removeAgent(fileDescriptor);
  }
}

