#ifndef SERVER_DISPATCHER_HPP
#define SERVER_DISPATCHER_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "dispatcher/i_server_dispatcher.hpp"
#include "service/agent_service.hpp"
#include "service/command_service.hpp"
#include "service/metrics_service.hpp"
#include "service/response_service.hpp"
#include "session_manager.hpp"

class ServerDispatcher : public IServerDispatcher {
 public:
  explicit ServerDispatcher(SessionManager& sessionManager,
                            IAgentService& agentService,
                            ICommandService& commandService,
                            IResponseService& responseService,
                            IMetricsService& metricsService)
      : sessionManager_(sessionManager),
        agentService_(agentService),
        commandService_(commandService),
        responseService_(responseService), 
        metricsService_(metricsService) {};

  ServerDispatcher(const ServerDispatcher&) = delete;
  ~ServerDispatcher() = default;
  ServerDispatcher& operator=(const ServerDispatcher&) = delete;

  void handleFrame(FrameTransport& agent, const Frame& frame) override;
  void sendCommand(AgentConnection& agent,
                   const CommandPayload& Command) override;
  SessionManager& getSessionManager() override { return sessionManager_; }
  void onAgentDisconnect(AgentConnection& agent) override;

 private:
  SessionManager& sessionManager_;
  // RepositoryManager& repositoryManager_;
  IAgentService& agentService_;
  ICommandService& commandService_;
  IResponseService& responseService_;
  IMetricsService& metricsService_;
  // ── Incoming message handlers ───────────────────────
  void onRegister(AgentConnection& agent,
                  const std::vector<std::uint8_t>& payload);
  void onDashboardRegister(AgentConnection& dashboard,
                           const std::vector<std::uint8_t>& payload);
  void onResponse(AgentConnection& agent,
                  const std::vector<std::uint8_t>& payload);
  void onData(AgentConnection& agent, const std::vector<std::uint8_t>& payload);

  void onDashboardCommand(AgentConnection& dashboard,
                          const std::vector<std::uint8_t>& payload);

  void onDisconnect(AgentConnection& connection);

  void sendDisconnect(AgentConnection& agent);

  // void registerDashboard(AgentConnection& dashboard,
  //                        const OsInfoPayload& dashboardInfo);

  // void registerAgent(AgentConnection& agent, const OsInfoPayload& agentInfo,
  //                    RegisterPayload& registerToSent);
};

#endif

// TODO DASHBOARD SHOULD GET THESE
// pendingResponses_;  // keyed by response id
// struct IncompleteResponse {
//   ResponsePayload baseResponse;
//   std::vector<std::uint8_t> data;  // indexed by chunk_index
//   // std::uint8_t chunksReceived = 0;
// };
// std::map<std::uint16_t, IncompleteResponse>
//     pendingResponses_;  // keyed by response id

// void processCompleteResponse(AgentConnection& agent,
//                              const ResponsePayload& response);
// void createResponseEntry(const ResponsePayload& response);
// void tryCompleteResponse(AgentConnection& agent,
//                          const ResponsePayload& response);
