#ifndef RESPONSE_REPOSITORY_HPP
#define RESPONSE_REPOSITORY_HPP

#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "repository/database_connection.hpp"

class IResponseRepository {
 public:
  virtual ~IResponseRepository() = default;

  // Returns the received_at timestamp assigned by the DB (DEFAULT NOW()),
  // so callers (ResponseService) can build a DashboardResponse to forward
  // immediately without a follow-up query.

  virtual DashboardResponse save(const ResponsePayload& response) = 0;

  // Latest N commands for one agent (dashboard history panel).
  virtual std::vector<DashboardResponse> findByCommandId(
      std::uint32_t commandId, int limit) = 0;
  virtual std::vector<DashboardResponse> findByAgentId(
      const std::string& agentId, int limit) = 0;
};

// command_repository.hpp
class ResponseRepository : public IResponseRepository {
 private:
  IDatabaseConnection& db_;  // same connection, different instance
  DashboardResponse rowToDashboardResponse(const pqxx::row& row);

 public:
  explicit ResponseRepository(IDatabaseConnection& db) : db_(db) {}

  DashboardResponse save(const ResponsePayload& response) override;

  //   std::vector<DashboardResponse> findByAgentId(int commandId);

  std::vector<DashboardResponse> findByCommandId(std::uint32_t commandId,
                                                 int limit = 50) override;
  std::vector<DashboardResponse> findByAgentId(const std::string& agentId,
                                               int limit = 50) override;
};

#endif