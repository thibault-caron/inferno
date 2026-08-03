# Agent Architecture

![Agent's architecture diagram](./agent_architecture.png)

## Overview

The agent is a single-threaded, non-blocking monitor that continuously:

- Listens for commands from the server
- Collects system metrics at regular intervals
- Automatically reconnects on disconnection

## Single-threaded Event Loop

The agent loop uses cross-platform I/O multiplexing:
- **Linux/macOS**: `poll()` syscall
- **Windows**: `WSAPoll()` (equivalent behavior, same pollfd struct)

No threads, no fork. Timing is purely driven by:
- **Connection state**: attempts reconnection on timeout
- **Incoming frames**: routes to dispatcher
- **Metrics timing**: independent check before processing events


The loop handles three roles with a single timeout:
- `heartbeatMs` — server silence timeout (when connected)
- `retryMs` — reconnection backoff interval (when disconnected)
- Metrics interval — next sample due time

When disconnected, the poller has no file descriptors registered. On Linux,
`poll(nullptr, 0, timeoutMs)` is valid POSIX and blocks for `timeoutMs` 
doing nothing—effectively a platform-native sleep without calling `::sleep()`. 
On Windows, the equivalent is `WSAPoll()` or an explicit `Sleep()` call.

This replaces sleep entirely and keeps the single event loop structure intact.

**Defaults (from main.cpp):**
- Heartbeat: 30 seconds (how long to wait for server data before heartbeat)
- Reconnection backoff: 10 seconds (wait time between connection attempts)
- Metrics sampling: configurable (typically 1-5 seconds per deployment)

## Dispatcher: Gate, Not Orchestrator

Unlike a typical server, the dispatcher does NOT orchestrate metrics.
It only:

- Routes incoming COMMAND frames
- Sets START_METRICS / STOP_METRICS flags in Metrics Controller

The agent loop independently checks:

```cpp
if (connected_ && metricsController_->isActive() && metricsController_->isDue())
  metricsController_->tick(session_);  // ← Loop, not Dispatcher
```

This means metrics flow independently of the dispatcher—even if command
routing fails, metrics keep streaming.

## AgentSession: State Holder + Connection Manager

AgentSession is more than just a socket wrapper. It tracks:

| State                       | Who sets                  | Who clears     | Purpose                            |
| --------------------------- | ------------------------- | -------------- | ---------------------------------- |
| `registered_`               | Dispatcher (sendRegister) | resetSession() | Did registration complete?         |
| `disconnectRequested_`      | Dispatcher (onDisconnect) | resetSession() | Clean close requested?             |
| `agentInfo_` (hostname, OS) | Dispatcher (getOsInfo)    | NEVER cleared  | Agent identity—survives reconnects |

## Disconnection flow

When the server sends DISCONNECT:

1. Dispatcher sets `disconnectRequested_ = true`
2. Loop checks the flag before processing events
3. Loop calls `session_.onDisconnect()` (cleanup)
4. Loop calls `session_.resetSession()` (this clears the flag + creates new socket)
5. Next iteration: fresh connection attempt

The key insight: `agentInfo_` survives `resetSession()` because the agent's
identity (hostname, architecture) never changes—only the socket does.

## Project structure
```
├── 📁 agent
│   ├── 📁 include
│   │   ├── 📁 dispatcher
│   │   │   ├── ⚡ agent_dispatcher.hpp
│   │   │   └── ⚡ i_agent_dispatcher.hpp
│   │   ├── 📁 metrics
│   │   │   ├── ⚡ i_metrics_scrapper.hpp
│   │   │   ├── ⚡ linux_metrics_scrapper.hpp
│   │   │   ├── ⚡ metrics_controller.hpp
│   │   │   └── ⚡ metrics_scrapper_factory.hpp
│   │   ├── 📁 system_monitor
│   │   │   ├── ⚡ i_system_monitor.hpp
│   │   │   ├── ⚡ linux_system_monitor.hpp
│   │   │   └── ⚡ system_monitor_factory.hpp
│   │   ├── ⚡ agent_loop.hpp
│   │   └── ⚡ agent_session.hpp
│   ├── 📁 src
│   │   ├── 📁 dispatcher
│   │   │   └── ⚡ agent_dispatcher.cpp
│   │   ├── 📁 metrics
│   │   │   ├── ⚡ linux_metrics_scrapper.cpp
│   │   │   ├── ⚡ metrics_controller.cpp
│   │   │   └── ⚡ metrics_scrapper_factory.cpp
│   │   ├── 📁 system_monitor
│   │   │   ├── ⚡ linux_system_monitor.cpp
│   │   │   └── ⚡ system_monitor_factory.cpp
│   │   ├── ⚡ agent_loop.cpp
│   │   ├── ⚡ agent_session.cpp
│   │   └── ⚡ main.cpp
│   ├── 📁 tests
│   │   ├── 📁 builders
│   │   │   └── ⚡ metrics_controller_test_factory.hpp
│   │   ├── 📁 metrics
│   │   │   ├── ⚡ linux_metrics_scrapper_test.cpp
│   │   │   └── ⚡ metrics_controller_test.cpp
│   │   ├── 📁 stubs
│   │   │   ├── ⚡ fake_metrics_scrapper.hpp
│   │   │   └── ⚡ fake_system_monitor.hpp
│   │   ├── 📁 system_monitor
│   │   │   └── ⚡ linux_system_monitor_test.cpp
│   │   └── ⚡ agent_dispatcher_test.cpp
│   └── 📄 CMakeLists.txt
``` 
### Key Files

- `agent_loop.hpp` — event loop + timing (see [Agent Loop](#single-threaded-event-loop))
- `dispatcher/` — routes COMMAND frames
- `metrics/` — independent metrics collection (see [Dispatcher](#dispatcher-gate-not-orchestrator))
- `system_monitor/` — OS info + process list + shell execution