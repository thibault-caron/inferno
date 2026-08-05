#include "repository/command_repository.hpp"

std::uint32_t CommandRepository::save(const DashboardCommand& cmd) {
  pqxx::params params;
  params.append(cmd.target);
  params.append(static_cast<int>(cmd.command.type));
  params.append(cmd.command.data);

  pqxx::result result = db_.executeParams(
      "INSERT INTO command_history (agent_id, command_type, "
      "command_data) VALUES ($1, $2, $3) RETURNING id",
      params);

  return static_cast<std::uint32_t>(result[0][0].as<int>());
}

std::vector<DashboardCommand> CommandRepository::findByAgentId(
    const std::string& agentId, int limit) {
  pqxx::params params;
  params.append(agentId);
  params.append(limit);

  pqxx::result rows = db_.executeParams(
      "SELECT id, command_type, command_data, sent_at FROM command_history"
      "WHERE agent_id = $1 ORDER BY created_at DESC LIMIT $2",
      params);

  std::vector<DashboardCommand> commands;
  commands.reserve(rows.size());
  for (const auto& row : rows) {
    commands.push_back(rowToDashboardCommand(row));
  }
  return commands;
}

CommandType CommandRepository::getCommandType(const std::uint32_t commandId) {
  pqxx::params params;
  params.append(commandId);

  pqxx::result rows = db_.executeParams(
      "SELECT command_type FROM command_history"
      "WHERE id = $1",
      params);

  if (rows.empty()) {
    throw std::runtime_error("Command not found");
  }

  return static_cast<CommandType>(rows[0]["command_type"].as<int>());
}

DashboardCommand CommandRepository::rowToDashboardCommand(
    const pqxx::row& row) {
  DashboardCommand cmd;
  cmd.sent_at = row["sent_at"].as<std::string>();  // type is TIMESTAMPTZ
  cmd.target =
      row["agent_id"].as<std::string>();  // target id = mac adress for now
  cmd.command.id = static_cast<uint32_t>(row["id"].as<long long>());
  cmd.command.type = static_cast<CommandType>(row["command_type"].as<int>());
  cmd.command.data = row["command_data"].as<std::string>();
  return cmd;
}