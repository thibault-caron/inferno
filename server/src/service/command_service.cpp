#include "service/command_service.hpp"

#include "exception/lptf_exception.hpp"

// Save in db + save in in memory command.id, target map
void CommandService::save(DashboardCommand& commandDashboard) {
  commandDashboard.command.id = repository_.save(commandDashboard);
  commandTargets_[commandDashboard.command.id] = commandDashboard.target;
}

// get target name from in memory map
std::string CommandService::getTarget(std::uint32_t commandId) {
  // return std::string();
  auto it =
      commandTargets_.find(commandId);  // used auto since it is an iterator
  if (it == commandTargets_.end()) {
    Logger::error("command service", "Unknown command id in response");
    // return;
    throw NotFound("target for command id :", std::to_string(commandId));
  }
  return it->second;
}

// Erase command.id and target string from map, in memory deletion only
void CommandService::deleteTarget(std::uint32_t commandId) {
  commandTargets_.erase(commandId);
}

// Todo : no op for now
std::vector<DashboardCommand> CommandService::findByAgentId(
    const std::string& agentId, int limit) {
  return std::vector<DashboardCommand>();
}

CommandType CommandService::getCommandType(const std::uint32_t commandId) {
  CommandType type = CommandType::UNKNOWN;
  try {
    type = repository_.getCommandType(commandId);
  } catch (const std::exception& ex) {
    Logger::error("Command service", "command id " + std::to_string(commandId) +
                                         " not found. " + ex.what());
  }
  return type;
}
