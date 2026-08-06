#include "service/response_service.hpp"

DashboardResponse ResponseService::save(const ResponsePayload& response) {
  // Let NotFound propagate: an orphan response (unknown/already-cleaned-up
  // command id) is a routing problem the caller needs to decide how to
  // handle (e.g. log it and send an ERROR back), not something this
  // service should silently swallow.

  // std::string target = commandService_.getTarget(response.id);
  // ResponsePayload payload = response;
  return repository_.save(response);

  // CommandType actualType = commandService_.getCommandType(response.id);

  // if (actualType != CommandType::UNKNOWN) {
  //   payload.type = actualType;
  // }

  // std::string receivedAt = repository_.save(payload);

  // DashboardResponse dashboardResponse;
  // dashboardResponse.target = target;
  // dashboardResponse.received_at = receivedAt;
  // dashboardResponse.response = payload;

  // // Last chunk: the command -> target mapping has served its purpose.
  // if (payload.chunk_index + 1 >= payload.total_chunks) {
  //   commandService_.deleteTarget(payload.id);
  // }

  // return dashboardResponse;
}

std::vector<DashboardResponse> ResponseService::findByCommandId(
    std::uint32_t commandId, int limit) {
  return repository_.findByCommandId(commandId, limit);
}

// Todo : history of responses, not implemented yet
std::vector<DashboardResponse> ResponseService::findByAgentId(
    const std::string& agentId, int limit) {
  return repository_.findByAgentId(agentId, limit);
}