#ifndef FAKE_RESPONSE_REPOSITORY_HPP
#define FAKE_RESPONSE_REPOSITORY_HPP

#include "repository/response_repository.hpp"

class FakeResponseRepository : public IResponseRepository {
 private:
  std::vector<DashboardResponse> saved_;
  std::string fixedReceivedAt_ = "2026-01-01 00:00:00+00";
  
  // Mock the command_history data for the JOIN
  std::map<std::uint32_t, std::pair<std::string, CommandType>> commands_;

 public:
  explicit FakeResponseRepository() {}

  DashboardResponse save(const ResponsePayload& response) override {
    DashboardResponse dr;
    dr.response = response;
    dr.received_at = fixedReceivedAt_;
    
    // Simulate the JOIN: look up command to get target + type
    auto it = commands_.find(response.id);
    if (it != commands_.end()) {
      dr.target = it->second.first;  // agent_id
      if (it->second.second != CommandType::UNKNOWN) {
        dr.response.type = it->second.second;
      }
    }
    
    saved_.push_back(dr);
    return dr;
  }

  std::vector<DashboardResponse> findByCommandId(std::uint32_t commandId,
                                                 int limit) override {
    std::vector<DashboardResponse> result;
    for (const auto& r : saved_) {
      if (r.response.id == commandId) result.push_back(r);
      if (static_cast<int>(result.size()) >= limit) break;
    }
    return result;
  }

  std::vector<DashboardResponse> findByAgentId(
      [[maybe_unused]] const std::string& agentId, int limit) override {
    std::vector<DashboardResponse> result(
        saved_.begin(),
        saved_.begin() +
            std::min(saved_.size(), static_cast<std::size_t>(limit)));
    return result;
  }

  // Test helpers
  const std::vector<DashboardResponse>& saved() const { return saved_; }
  void clear() { saved_.clear(); }
  
  // Mock the command_history data so the JOIN works in tests
  void mockCommand(std::uint32_t commandId, const std::string& agentId,
                   CommandType type = CommandType::UNKNOWN) {
    commands_[commandId] = {agentId, type};
  }
};

#endif