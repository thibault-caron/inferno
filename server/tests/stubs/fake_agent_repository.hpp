#ifndef FAKE_AGENT_REPOSITORY_HPP
#define FAKE_AGENT_REPOSITORY_HPP

#include <iostream>
#include <chrono>

#include "repository/agent_repository.hpp"
// agent_repository.hpp
class FakeAgentRepository : public IAgentRepository {
 private:
  std::map<std::string, RegisterPayload> agents_;

 public:
  explicit FakeAgentRepository() {}

  void save(const RegisterPayload& agent) override {
    agents_[agent.id] = agent;  // Upsert
  };
  void setLastSeen(const std::string& id) override {
    auto it = agents_.find(id);
    if (it != agents_.end()) {
      std::ostringstream oss;
      oss << std::chrono::system_clock::now();
      it->second.last_seen = oss.str();
      std::cout << "updated last seen of: " << id << " at " << it->second.last_seen
                << "\n";
    }
  };
  std::vector<RegisterPayload> findAll() override {
    std::vector<RegisterPayload> result;
    for (const auto& [id, agent] : agents_) {
      result.push_back(agent);
    }
    return result;
  };
  std::optional<RegisterPayload> findById(const std::string& id) override {
    auto it = agents_.find(id);
    if (it != agents_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  std::optional<std::string> getLastSeen(const std::string& id) override {
    auto it = agents_.find(id);
    if (it != agents_.end()) {
      return it->second.last_seen;
    }
    return std::nullopt;
  }
 
  std::optional<std::string> getRegisteredAt(const std::string& id) override {
    auto it = agents_.find(id);
    if (it != agents_.end()) {
      return it->second.registered_at;
    }
    return std::nullopt;
  }


  // Test-only helper: check what was stored
  const std::map<std::string, RegisterPayload>& getStoredAgents() const {
    return agents_;
  };
  // etc.
};

#endif