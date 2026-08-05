
#include <cstdio>
#include <iostream>

#include "dispatcher/server_dispatcher.hpp"
#include "env_helper.hpp"
#include "poller/epoller.hpp"
#include "reactor.hpp"
#include "repository/command_repository.hpp"
#include "repository/database_connection.hpp"
#include "repository/metrics_repository.hpp"
#include "service/agent_service.hpp"
#include "service/command_service.hpp"
#include "service/metrics_service.hpp"
#include "tcp_server.hpp"
#include "logger.hpp" 

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);
  Epoller poller;
  if (!poller.isValid()) {
    std::cerr << "[server] Failed to create epoll instance\n";
    return 1;
  }

  const uint16_t serverPort = EnvHelper::resolvePort();
  Logger::info("server main", "SERVER_PORT = " + std::to_string(serverPort));
  
  bool encryption = EnvHelper::resolveTlsEnabled();
  TcpServer server(serverPort, encryption);
  if (!server.start()) {
    std::cerr << "[server] Failed to bind/listen on port " << serverPort
              << '\n';
    return 1;
  }

  server.setNonBlocking();

  SessionManager sessionManager;
  DatabaseConnection db;

  AgentRepository agentRepository(db);
  AgentService agentService(agentRepository, sessionManager);

  CommandRepository commandRepository(db);
  CommandService commandService(commandRepository, sessionManager);

  ResponseRepository responseRepository(db);
  ResponseService responseService(responseRepository);

  MetricsRepository metricsRepository(db);
  MetricsService metricsService(metricsRepository, sessionManager);
  // RepositoryManager repositoryManager;
  ServerDispatcher dispatcher(sessionManager, agentService, commandService,
                              responseService, metricsService);
  Reactor reactor(server, dispatcher, poller, sessionManager);
  reactor.run();
  return 0;
}