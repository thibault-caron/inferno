#ifndef FAKE_COMMAND_REPOSITORY_HPP
#define FAKE_COMMAND_REPOSITORY_HPP

#include "repository/command_repository.hpp"

class FakeCommandRepository : public ICommandRepository {
 private:
  std::vector<DashboardCommand> commands_;
  int nextId_ = 1;

 public:
  explicit FakeCommandRepository() {}

  std::uint32_t save(const DashboardCommand& cmd) override {
    commands_.push_back(cmd);
    return nextId_++;  // Return auto-incremented ID
  };

  // TODO findAll too?
  std::vector<DashboardCommand> findByAgentId(const std::string& agentId,
                                              int limit = 50) override {
    std::vector<DashboardCommand> result;
    int count = 0;

    // Return most recent first
    for (auto it = commands_.rbegin(); it != commands_.rend() && count < limit;
         ++it) {
      if (it->target == agentId) {
        result.push_back(*it);
        count++;
      }
    }
    return result;
  };
  // Test-only helper: check what was stored
  const std::vector<DashboardCommand>& getStoredCommands() const {
    return commands_;
  };

  // Test-only: clear state between tests
  void clear() {
    commands_.clear();
    nextId_ = 1;
  };

  CommandType getCommandType(std::uint32_t commandId) override {
    for (const auto& cmd : commands_) {
      if (cmd.command.id == commandId) return cmd.command.type;
    }

    return CommandType::UNKNOWN;
  }
};

#endif