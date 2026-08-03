# Protocol Message Flows

This page documents the key message sequences in Inferno using UML sequence diagrams.
For more details on protocol, see [lptf binary protocol](../project/lptf_binary_protocol.md).

## Registration

### Agent and Dashboard Registration

![Register sequence diagram](./register_sequence.png)

#### Agent

When an agent connects, it sends OS information (hostname, architecture, MAC).
The server stores it and notifies the dashboard.

#### Dashboard

When the dashboard connects, it identifies itself and requests the list of all known agents.
The server marks which agents are currently online.

---

### Command Execution

![Command response sequence diagram](./command_response_sequence.png)

The dashboard sends a command targeting a specific agent. The server maps the **command ID** to the target, forwards to the agent, and routes the response back to the dashboard using the **same ID**.

> Special case: `START_METRICS` and `STOP_METRICS` don't return traditional responses—
they use DATA frames instead.

---

### Unsolicited Data (Metrics)

![Data sequence diagram](./data_sequence.png)

Agents push metrics to the server independently of commands. The server saves
the metrics and forwards them to the dashboard for real-time display.

---

### Disconnection

![Disconnect sequence diagram](./disconnect_sequence.png)

Can be initiated by dashboard (clean shutdown) or detected by the server
(socket close). In both cases, the server notifies the dashboard and cleans
up transient state.
