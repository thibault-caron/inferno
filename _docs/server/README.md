# Server Architecture

![Server's architecture diagram](./server_architecture.png)

## Overview

The server is the central coordinator of the Inferno platform. It is responsible for maintaining all active network sessions, routing messages between the dashboard and connected agents, and persisting operational data to the database.

Unlike the monitoring agents, the server does not collect system information itself. Instead, it coordinates communication, routing, and persistence between the different components of the platform.

The server is implemented as a **single-threaded event-driven application** based on Linux `epoll`. A single reactor loop manages every connected socket, allowing multiple agents and one dashboard to communicate concurrently without dedicating a thread to each connection.

Its main responsibilities are:

- accepting and managing multiple simultaneous TCP connections,
- distinguishing dashboard and agent sessions during registration,
- routing commands from the dashboard to their target agent,
- assigning database-generated command identifiers,
- forwarding agent responses back to the dashboard,
- persisting commands, responses, registrations, and collected metrics,
- maintaining the runtime state of connected clients.

## Command Routing & Response Mapping

The dashboard never communicates directly with agents. Instead:

1. **Dashboard sends a command** targeting a specific agent
2. **Server persists the command** to the database
3. **Database generates a unique command ID**
4. **Server records the mapping**: `command.id → target agent`
5. **Server forwards the command** to the target agent
6. **Agent executes and sends response** (with the same command ID)
7. **Server uses the response ID** to look up the original target
8. **Server wraps the response** as `DashboardResponse` and forwards to dashboard
9. **After the final response chunk**, the mapping is deleted from memory

The in-memory mapping (step 4) is transient, it exists only while the command is active. This allows agents to be stateless about the dashboard while
ensuring asynchronous responses are routed back reliably.

This mechanism allows agents to remain unaware of the dashboard while enabling reliable routing of asynchronous responses.

For protocol details (REGISTER, DASHBOARD_REGISTER, COMMAND, RESPONSE, DATA),
see [Communication Protocol](../../_docs/project/lptf_binary_protocol.md).

## Runtime architecture

The server follows a **domain-driven layered architecture**:

- **Reactor** receives network events from `epoll`, accepts new connections, extracts complete protocol frames, and dispatches them for processing.
- **SessionManager** maintains every active connection, distinguishes the dashboard from monitoring agents, and provides runtime lookup tables (see [Session management](#session-management)).
- **ServerDispatcher** inspects each decoded frame and delegates processing to the appropriate service.
- **Services** implement the business logic for agent registration, command routing, response handling, and metrics persistence.
- **Repositories** encapsulate persistence using PostgreSQL/TimescaleDB.
- **AgentConnection** represents a connected endpoint and stores the runtime state associated with that connection (see [Agent connection](#agent-connection)).

## Session management

Every incoming TCP connection is initially represented as an `AgentConnection`. During registration, the server determines whether the connection belongs to a monitoring agent or the dashboard.

The `SessionManager` maintains three in-memory lookup tables (maps):

- `fd -> AgentConnection`
- `agent id (MAC) -> file descriptor`
- `file descriptor -> agent id (MAC)`

The three lookup tables allow different layers of the server to work with the identifier most natural to them. The reactor operates on file descriptors, protocol routing generally uses persistent agent identifiers, while network operations require direct access to the associated `AgentConnection`.

## AgentConnection: Runtime Connection State

`AgentConnection` represents a connected endpoint throughout its lifetime. Although it inherits the transport layer responsible for frame parsing and socket I/O, it also owns the runtime state required by the server.

Each connection stores:

- the underlying TCP connection,
- the receive buffer used for frame extraction,
- the persistent agent identifier (currently the MAC address),
- the registration payload describing the connected system,
- a registration flag indicating whether protocol negotiation has completed.

The registration flag enforces the protocol handshake. Newly connected clients are considered unregistered and are only allowed to send a `REGISTER` or `DASHBOARD_REGISTER` message. Any other message type is rejected, preventing unauthenticated communication before the endpoint has been identified.

## Database

The server persists operational data using PostgreSQL with the TimescaleDB extension.

Each repository shares the same database connection while remaining responsible for its own table(s), keeping persistence concerns separated from the server's networking logic.

Regular relational tables store relatively static information:

- agents
- command_history
- responses

Time-series data is stored using TimescaleDB hypertables:

- metrics
- metrics_disk
- metrics_net

This separation allows command and registration history to benefit from relational storage while efficiently handling the continuous stream of monitoring samples generated by connected agents.

see database schema [init.sql](../../init.sql)

## Project structure

```
Server
  ├── 📁 include
  │   ├── 📁 dispatcher
  │   │   ├── ⚡ i_server_dispatcher.hpp
  │   │   └── ⚡ server_dispatcher.hpp
  │   ├── 📁 poller
  │   │   └── ⚡ epoller.hpp
  │   ├── 📁 repository
  │   │   ├── ⚡ agent_repository.hpp
  │   │   ├── ⚡ command_repository.hpp
  │   │   ├── ⚡ database_connection.hpp
  │   │   ├── ⚡ metrics_repository.hpp
  │   │   └── ⚡ response_repository.hpp
  │   ├── 📁 service
  │   │   ├── ⚡ agent_service.hpp
  │   │   ├── ⚡ command_service.hpp
  │   │   ├── ⚡ metrics_service.hpp
  │   │   └── ⚡ response_service.hpp
  │   ├── ⚡ agent_connection.hpp
  │   ├── ⚡ reactor.hpp
  │   ├── ⚡ session_manager.hpp
  │   └── ⚡ tcp_server.hpp
  ├── 📁 src
  │   ├── 📁 dispatcher
  │   │   └── ⚡ server_dispatcher.cpp
  │   ├── 📁 poller
  │   │   └── ⚡ epoller.cpp
  │   ├── 📁 repository
  │   │   ├── ⚡ agent_repository.cpp
  │   │   ├── ⚡ command_repository.cpp
  │   │   ├── ⚡ database_connection.cpp
  │   │   ├── ⚡ metrics_repository.cpp
  │   │   └── ⚡ response_repository.cpp
  │   ├── 📁 service
  │   │   ├── ⚡ agent_service.cpp
  │   │   ├── ⚡ command_service.cpp
  │   │   ├── ⚡ metrics_service.cpp
  │   │   └── ⚡ response_service.cpp
  │   ├── ⚡ agent_connection.cpp
  │   ├── ⚡ main.cpp
  │   ├── ⚡ reactor.cpp
  │   ├── ⚡ session_manager.cpp
  │   └── ⚡ tcp_server.cpp
  ├── 📁 tests
  │   ├── 📁 stubs
  │   │   ├── ⚡ fake_agent_repository.hpp
  │   │   ├── ⚡ fake_command_repository.hpp
  │   │   ├── ⚡ fake_metrics_repository.hpp
  │   │   └── ⚡ fake_response_repository.hpp
  │   ├── ⚡ epoller_test.cpp
  │   ├── ⚡ reactor_integration_test.cpp
  │   ├── ⚡ server_dispatcher_test.cpp
  │   └── ⚡ tcp_server_test.cpp
  └── 📄 CMakeLists.txt
```
