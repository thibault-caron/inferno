#include "repository/response_repository.hpp"

#include "exception/lptf_exception.hpp"

// std::string ResponseRepository::save(const ResponsePayload& response) {
//   pqxx::params params;
//   params.append(response.id);
//   params.append(static_cast<int>(response.status));
//   params.append(static_cast<int>(response.total_chunks));
//   params.append(static_cast<int>(response.chunk_index));
//   params.append(
//       pqxx::bytes_view(reinterpret_cast<const
//       std::byte*>(response.data.data()),
//                        response.data.size()));

//   pqxx::result result = db_.executeParams(
//       "INSERT INTO responses "
//       "  (command_id, status, total_chunks, chunk_index, chunk_data) "
//       "VALUES ($1, $2, $3, $4, $5) RETURNING received_at",
//       params);
//   return result[0][0].as<std::string>();
// }

DashboardResponse ResponseRepository::save(const ResponsePayload& response) {
  // Query 1: Get target + commandType from command_history
  pqxx::result ctx = db_.executeParams(
      "SELECT c.agent_id, c.command_type "
      "FROM command_history c "
      "WHERE c.id = $1",
      {response.id});  // response.id is the command_id

  if (ctx.empty()) {
    // throw NotFound("Command id in response table : " +
    //                std::to_string(response.id));
    throw NotFound("command_id", std::to_string(response.id));
  }
  // Assemble everything
  DashboardResponse resp;

  resp.target = ctx[0]["agent_id"].as<std::string>();
  resp.response = response;
  CommandType actualType =
      static_cast<CommandType>(ctx[0]["command_type"].as<int>());
  if (actualType != CommandType::UNKNOWN) {
    resp.response.type = actualType;
  }

  pqxx::params params;
  params.append(response.id);
  params.append(static_cast<int>(response.status));
  params.append(static_cast<int>(response.total_chunks));
  params.append(static_cast<int>(response.chunk_index));
  params.append(
      pqxx::bytes_view(reinterpret_cast<const std::byte*>(response.data.data()),
                       response.data.size()));
  // Query 2: INSERT response and get received_at
  pqxx::result result = db_.executeParams(
      "INSERT INTO responses "
      "(command_id, status, total_chunks, chunk_index, chunk_data) "
      "VALUES ($1, $2, $3, $4, $5) "
      "RETURNING id, received_at",
      params);

  resp.received_at = result[0]["received_at"].as<std::string>();

  return resp;
}

std::vector<DashboardResponse> ResponseRepository::findByCommandId(
    std::uint32_t commandId, int limit) {
  pqxx::params params;
  params.append(static_cast<int>(commandId));
  params.append(limit);

  pqxx::result rows = db_.executeParams(
      "SELECT r.id, r.status, r.total_chunks, r.chunk_index, r.chunk_data, "
      "r.received_at, c.agent_id "
      "FROM responses r "
      "JOIN command_history c ON r.command_id = c.id "
      "WHERE r.command_id = $1 "
      "ORDER BY r.chunk_index ASC LIMIT $2",
      params);

  std::vector<DashboardResponse> responses;
  responses.reserve(rows.size());
  for (const auto& row : rows) {
    responses.push_back(rowToDashboardResponse(row));
  }
  return responses;
}

std::vector<DashboardResponse> ResponseRepository::findByAgentId(
    const std::string& agentId, int limit) {
  pqxx::params params;
  params.append(agentId);
  params.append(limit);

  pqxx::result rows = db_.executeParams(
      "SELECT r.id, r.status, r.total_chunks, r.chunk_index, r.chunk_data, "
      "r.received_at, c.agent_id "
      "FROM responses r "
      "JOIN command_history c ON r.command_id = c.id "
      "WHERE c.agent_id = $1 "
      "ORDER BY r.chunk_index ASC LIMIT $2",
      params);

  std::vector<DashboardResponse> responses;
  responses.reserve(rows.size());
  for (const auto& row : rows) {
    responses.push_back(rowToDashboardResponse(row));
  }
  return responses;
}

DashboardResponse ResponseRepository::rowToDashboardResponse(
    const pqxx::row& row) {
  DashboardResponse resp;
  resp.received_at = row["received_at"].as<std::string>();
  resp.target = row["agent_id"].as<std::string>();

  resp.response.id = static_cast<std::uint32_t>(row["id"].as<long long>());
  resp.response.status = static_cast<ResponseStatus>(row["status"].as<int>());
  resp.response.total_chunks =
      static_cast<std::uint8_t>(row["total_chunks"].as<int>());
  resp.response.chunk_index =
      static_cast<std::uint8_t>(row["chunk_index"].as<int>());

  auto data = row["chunk_data"].as<std::string>();
  resp.response.data.assign(data.begin(), data.end());
  return resp;
}