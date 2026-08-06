#include "repository/agent_repository.hpp"

#include "time/time_converter.hpp"
#include "logger.hpp"

// EXCLUDED. --> new value we try to insert in case of conflict, i.e. the new
// values we want to update. (agents.value --> old value)
void AgentRepository::save(const RegisterPayload& agent) {
  pqxx::params params;
  params.append(agent.id);
  params.append(agent.system.hostname);
  params.append(static_cast<int>(agent.system.os_type));
  params.append(static_cast<int>(agent.system.arch));
  params.append(agent.system.os_version);
  params.append(agent.system.current_user);
  params.append(agent.system.ip);
  params.append(agent.system.mac);

  db_.executeParams(
      "INSERT INTO agents "
      "  (id, hostname, os_type, architecture, os_version, current_username, "
      "ip_address, mac_address) "
      "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) "
      "ON CONFLICT (id) DO UPDATE SET "
      "  hostname = EXCLUDED.hostname, "
      "  os_type = EXCLUDED.os_type, "
      "  architecture = EXCLUDED.architecture, "
      "  os_version = EXCLUDED.os_version, "
      "  current_username = EXCLUDED.current_username, "
      "  ip_address = EXCLUDED.ip_address, "
      "  mac_address = EXCLUDED.mac_address, "
      "  last_seen = NOW()",
      params);
}

void AgentRepository::setLastSeen(const std::string& id) {
  pqxx::params params;
  // params.append(timestampIso);
  params.append(id);

  db_.executeParams(
      "UPDATE agents "
      "SET last_seen = NOW() "
      "WHERE id = $1",
      params);
}

std::vector<RegisterPayload> AgentRepository::findAll() {
  pqxx::result rows = db_.execute(
      "SELECT id, registered_at, last_seen, hostname, os_type, architecture, "
      "os_version, current_username, ip_address, mac_address "
      "FROM agents ORDER BY hostname");

  std::vector<RegisterPayload> agents;
  agents.reserve(rows.size());
  for (const auto& row : rows) {
    agents.push_back(rowToRegisterPayload(row));
  }
  return agents;
}

std::optional<RegisterPayload> AgentRepository::findById(
    const std::string& id) {
  Logger::info("agent repository", "in find by id");

  pqxx::params params;
  params.append(id);

  pqxx::result rows = db_.executeParams(
      "SELECT id, registered_at, last_seen, hostname, os_type, architecture, "
      "os_version, current_username, ip_address, mac_address "
      "FROM agents WHERE id = $1",
      params);

  if (rows.empty()) {
    return std::nullopt;
  }
  return rowToRegisterPayload(rows[0]);
}

std::optional<std::string> AgentRepository::getLastSeen(const std::string& id) {
  pqxx::params params;
  params.append(id);

  pqxx::result rows =
      db_.executeParams("SELECT last_seen FROM agents WHERE id = $1", params);

  if (rows.empty()) {
    return std::nullopt;
  }
  return TimeConverter::timestamptzToIso(
      rows[0]["last_seen"].as<std::string>());
}

std::optional<std::string> AgentRepository::getRegisteredAt(
    const std::string& id) {
  pqxx::params params;
  params.append(id);

  pqxx::result rows = db_.executeParams(
      "SELECT registered_at FROM agents WHERE id = $1", params);

  if (rows.empty()) {
    return std::nullopt;
  }
  return TimeConverter::timestamptzToIso(
      rows[0]["registered_at"].as<std::string>());
}

RegisterPayload AgentRepository::rowToRegisterPayload(const pqxx::row& row) {
  RegisterPayload agent;
  agent.id = row["id"].as<std::string>();
  agent.registered_at =
      TimeConverter::timestamptzToIso(row["registered_at"].as<std::string>());
  agent.last_seen =
      TimeConverter::timestamptzToIso(row["last_seen"].as<std::string>());
  agent.system.os_type = static_cast<OSType>(row["os_type"].as<int>());
  agent.system.arch = static_cast<ArchType>(row["architecture"].as<int>());
  agent.system.hostname = row["hostname"].as<std::string>();
  agent.system.os_version = row["os_version"].as<std::string>();
  agent.system.current_user = row["current_username"].as<std::string>();
  agent.system.ip = row["ip_address"].as<std::string>();
  agent.system.mac = row["mac_address"].as<std::string>();
  // MAC address is the agent identity == id today,but might change later
  return agent;
}
