#ifndef RESPONSE_SERVICE_HPP
#define RESPONSE_SERVICE_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "repository/response_repository.hpp"

class IResponseService {
 public:
  virtual ~IResponseService() = default;

  virtual DashboardResponse save(const ResponsePayload& response) = 0;

  virtual std::vector<DashboardResponse> findByCommandId(
      std::uint32_t commandId, int limit = 50) = 0;
  virtual std::vector<DashboardResponse> findByAgentId(
      const std::string& agentId, int limit = 50) = 0;
};

class ResponseService : public IResponseService {
 private:
  IResponseRepository& repository_;

 public:
  explicit ResponseService(IResponseRepository& repository)
      : repository_(repository) {}

  DashboardResponse save(const ResponsePayload& response) override;

  std::vector<DashboardResponse> findByCommandId(std::uint32_t commandId,
                                                 int limit = 50) override;
  std::vector<DashboardResponse> findByAgentId(const std::string& agentId,
                                               int limit = 50) override;
};

#endif