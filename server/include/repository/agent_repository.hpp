#ifndef AGENT_REPOSITORY_HPP
#define AGENT_REPOSITORY_HPP

#include <optional>
#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "repository/database_connection.hpp"

class IAgentRepository {
 public:
  virtual ~IAgentRepository() = default;

  // Upsert — creates agent on first REGISTER, updates fields on reconnect.
  virtual void save(const RegisterPayload& agent) = 0;
  virtual void setLastSeen(const std::string& id) = 0;
  virtual std::vector<RegisterPayload> findAll() = 0;
  virtual std::optional<RegisterPayload> findById(const std::string& id) = 0;
  virtual std::optional<std::string> getLastSeen(const std::string& id) = 0;
  virtual std::optional<std::string> getRegisteredAt(const std::string& id) = 0;

};

// agent_repository.hpp
class AgentRepository : public IAgentRepository {
 private:
  IDatabaseConnection& db_;  // shared reference
  RegisterPayload rowToRegisterPayload(const pqxx::row& row);

 public:
  explicit AgentRepository(IDatabaseConnection& db) : db_(db) {}

  void save(const RegisterPayload& agent) override;
  void setLastSeen(const std::string& id) override;
  std::vector<RegisterPayload> findAll() override;
  std::optional<RegisterPayload> findById(const std::string& id) override;
  std::optional<std::string> getLastSeen(const std::string& id) override;
  std::optional<std::string> getRegisteredAt(const std::string& id) override;
  // etc.
};

#endif