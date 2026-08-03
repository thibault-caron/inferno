# LPTF Binary Protocol

## 1. Overview

LPTF is a custom binary protocol for agent-server-dashboard communication.

- Agent connects and registers itself.
- Server sends COMMANDS to agents on behalf of the dashboard.
- Agent executes commands and sends RESPONSES (possibly chunked).
- Agent may send unsolicited DATA (metrics, health checks).
- Dashboard connects and registers itself.
- Dashboard sends COMMANDS targeting specific agents via MAC address.
- Dashboard receives RESPONSES and DATA forwarded by the server.
- TCP sockets are used as the transport.

**Design goals:**

- Cross-platform
- Lightweight
- Extendable

---

## 2. Transport

- TCP sockets (`SOCK_STREAM`)
- TCP provides reliability, ordered delivery, and byte-stream semantics.
- The protocol is responsible for framing: parsing the byte stream into discrete messages, including reassembly of chunked responses.

---

## 3. Message Format

All messages share a fixed 8-byte header, immediately followed by a payload.

```
[Identifier][Version][Type][Size]
  4 bytes     1 byte   1B    2B
```

| Field      | Size | Type    | Description                       |
| ---------- | ---- | ------- | --------------------------------- |
| Identifier | 4B   | char[4] | ASCII magic: `"LPTF"`             |
| Version    | 1B   | uint8   | Protocol version (current: `1`)   |
| Type       | 1B   | uint8   | Message type (see §3.1)           |
| Size       | 2B   | uint16  | Payload length in bytes (0–65535) |

All integers are **big-endian**. All strings are **UTF-8**, length-prefixed, no null terminator.

### 3.1 Message Types

| Value | Name               | Direction                       | Description                       |
| ----- | ------------------ | ------------------------------- | --------------------------------- |
| 0     | REGISTER           | Agent → Server                  | Agent registration                |
| 1     | DASHBOARD_REGISTER | Dashboard → Server              | Dashboard registration            |
| 2     | DATA               | Agent → Server → Dashboard      | Unsolicited data (metrics, etc.)  |
| 3     | COMMAND            | Dashboard → Server → Agent      | Instruction to execute            |
| 4     | RESPONSE           | Agent → Server → Dashboard      | Result of a COMMAND               |
| 5     | DISCONNECT         | Any → Server, or Server → Agent | Graceful disconnection (see §4.6) |
| 6     | ERROR              | Server → Any                    | Error notification                |

---

## 4. Payload Structures

### 4.1 REGISTER

Sent by agent immediately after connecting. Must be the first message.

```
uint8_t  os_type
uint8_t  arch
uint16_t hostname_len
uint16_t os_version_len
uint16_t current_user_len
uint16_t ip_len
uint16_t mac_len
char     hostname[hostname_len]
char     os_version[os_version_len]
char     current_user[current_user_len]
char     ip[ip_len]
char     mac[mac_len]
```

```cpp
struct OsInfoPayload {
    uint8_t  os_type;           // 0=Windows, 1=Linux, 2=macOS
    uint8_t  arch;              // 0=x86, 1=x64, 2=ARM
    string    hostname;
    string     os_version;
    string     current_user;
    string     ip;
    string     mac;      // e.g. "aa:bb:cc:dd:ee:ff" — always 17 bytes
};
```

Fixed header size: `2 × sizeof(uint8_t) + 5 × sizeof(uint16_t) = 12 bytes`

Rules:

- Server ignores all messages until REGISTER (or DASHBOARD_REGISTER) is received.
- Invalid or missing REGISTER → server may close the connection.
- The MAC address is used as the persistent agent identity across reconnections.

DASHBOARD_REGISTER uses the same payload structure as REGISTER.
The message type in the header distinguishes agent from dashboard.

---

### 4.2 COMMAND

Sent by server to agent after routing a dashboard request.

```
uint32_t id
uint8_t  type
uint16_t data_len
char     data[data_len]
```

```cpp
struct CommandPayload {
    uint32_t id;            // server-generated unique command id
    uint8_t  type;          // see command types below
    string   data; // none unless type == SHELL
};
```

Fixed size: `sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t) = 7 bytes`

#### Command Types

| Value | Name              | Description                                                       |
| ----- | ----------------- | ----------------------------------------------------------------- |
| 0     | OS_INFO           | Request OS and hardware information                               |
| 1     | RUNNING_PROCESSES | Request current process list                                      |
| 2     | SHELL             | Execute a shell command (`data` field carries the command string) |
| 3     | START_METRICS     | Start periodic metrics sampling                                   |
| 4     | STOP_METRICS      | Stop periodic metrics sampling                                    |

Note: The dashboard does not generate the command `id`. It leaves it at `0`; the server assigns the actual id before forwarding to the agent.

---

### 4.3 RESPONSE

Sent by agent after executing a COMMAND. Supports chunking for large outputs.

```
uint32_t id
uint8_t  status
uint8_t  total_chunks
uint8_t  chunk_index
uint16_t data_len
uint8_t  data[data_len]
```

```cpp
struct ResponsePayload {
    uint32_t id;            // matches the command id this responds to
    uint8_t  status;        // 0=OK, 1=ERROR
    uint8_t  total_chunks;  // total number of chunks for this response
    uint8_t  chunk_index;   // 0-based index of this chunk
    string  data;
};
```

Fixed size: `sizeof(uint32_t) + sizeof(uint16_t) + 3 × sizeof(uint8_t) = 9 bytes`

#### Chunking

The server does **not** reassemble chunks. Each chunk is forwarded individually to the dashboard as a `DashboardResponse`. The dashboard is responsible for reassembly using `id`, `chunk_index`, and `total_chunks`.

Maximum data per chunk:

```
max_data = 65535 - RESPONSE_FIXED_BYTES
         = 65535 - 26
         = 65509 bytes
```

`RESPONSE_FIXED_BYTES = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) * 3 + MAC_SIZE`

> The agent reserves `MAC_SIZE = 17` bytes in each chunk to allow the server to prepend the agent identity string when wrapping as `DashboardResponse`. The agent does not know its own identity, the server assigns and inserts it. Today the identity is the MAC address (17 bytes); this may change in future versions as long as the identity size stays ≤ 17 bytes.

#### Process Info (RUNNING_PROCESSES response data)

```
uint32_t pid;
float    cpu_percent;   // 0.0–100.0, lifetime average
uint64_t mem_bytes;     // resident memory in kB (VmRSS)
uint16_t name_len;
char     name[name_len];
```

```cpp
struct ProcessInfo {
    uint32_t pid;
    float    cpu_percent;   // 0.0–100.0, lifetime average
    uint64_t mem_bytes;     // resident memory in kB (VmRSS)
    string     name;
};
```

`response.data` layout for RUNNING_PROCESSES:

```
uint16_t process_count
ProcessInfo[process_count]
```

---

### 4.4 DATA

Sent by agent without a prior COMMAND (proactive push). Also used by server to push agent info to dashboard.

```
uint8_t  subtype
uint16_t data_len
uint8_t  data[data_len]
```

```cpp
struct DataPayload {
    uint8_t  subtype;
    string  data;
};
```

#### Data Subtypes

| Value | Name           | Description                                                                 |
| ----- | -------------- | --------------------------------------------------------------------------- |
| 0     | METRICS_SAMPLE | Periodic system metrics snapshot (CPU, memory, disk, network)               |
| 1     | HEALTH_CHECK   | Agent heartbeat                                                             |
| 2     | REGISTRATION   | Server → Dashboard: new agent just connected (single RegisterPayload)       |
| 3     | AGENTS         | Server → Dashboard: full list of connected agents on dashboard registration |
| 4     | UNKNOWN        | Unrecognized subtype                                                        |

#### 4.4.1 Metrics Sample

```cpp
struct CpuSample {
    float   total_percent;
    uint8_t core_number;
    float   per_core[core_number];
};

struct MemSample {
    uint64_t phys_total;
    uint64_t phys_used;
    uint64_t phys_available;
    uint64_t swap_total;
    uint64_t swap_used;
};

struct DiskSample {
    float    read_bytes_per_sec;
    float    write_bytes_per_sec;
    uint16_t device_len;
    char     device[device_len];    // e.g. "sda", "C:"
};

struct NetSample {
    float    rx_bytes_per_sec;
    float    tx_bytes_per_sec;
    uint16_t iface_len;
    char     iface[iface_len];      // e.g. "eth0", "Ethernet"
};

struct MetricsSample {
    CpuSample  cpu;
    MemSample  mem;
    uint16_t   timestamp_len;
    uint8_t    disk_count;
    uint8_t    interface_count;
    char       timestamp[timestamp_len];
    DiskSample disks[disk_count];
    NetSample  interfaces[interface_count];
};
```

---

### 4.5 ERROR

Sent by server to report a protocol or routing error.

```
uint8_t  code
uint16_t message_len
char     message[message_len]
```

```cpp
struct ErrorPayload {
    uint8_t  code;
    string     message;
};
```

#### Error Codes

| Code | Name             | Description                                           |
| ---- | ---------------- | ----------------------------------------------------- |
| 0    | UNKNOWN_TYPE     | Unrecognized message type                             |
| 1    | INVALID_FORMAT   | Malformed payload                                     |
| 2    | UNKNOWN_COMMAND  | Command type not recognized or target agent not found |
| 3    | EXECUTION_FAILED | Agent failed to execute command                       |
| 4    | SIZE_EXCEEDED    | Payload exceeds maximum size                          |
| 5    | INVALID_TYPE     | Invalid type sent (wrong direction)                   |
| 6    | NOT_IMPLEMENTED  | Planned but not implemented yet                       |

---

### 4.6 DISCONNECT

The DISCONNECT message type covers three distinct flows depending on sender and payload.

#### Agent → Server (graceful shutdown, no payload)

Agent is shutting down normally (e.g. host machine shutdown). Warns server before closing the socket.

```
[DISCONNECT header | size=0]
```

No payload. Server acknowledges by removing the agent from its session table. No response is sent.

#### Dashboard → Server (disconnect request, with payload)

Dashboard requests that the server disconnect a specific agent.

```
[DISCONNECT header | size = 2 + target_len]
uint16_t target_len
char     target[target_len]   // agent MAC address, e.g. "aa:bb:cc:dd:ee:ff"
```

```cpp
struct DashboardDisconnect {
    std::string target;   // agent MAC address
};
```

Server parses the target, looks up the agent, and forwards a no-payload DISCONNECT frame to that agent. No confirmation is sent back to the dashboard.

#### Server → Agent (disconnect order, no payload)

```
[DISCONNECT header | size=0]
```

Agent receives this, performs cleanup (stops metrics stream, etc.), and closes the socket. The agent ignores any payload if present.

---

## 5. Dashboard ↔ Server Protocol

The dashboard never sends raw agent-level messages. All dashboard messages wrap agent payloads with a `target` field identifying the destination agent by MAC address.

### 5.1 Wire Format (dashboard → server)

```
uint16_t target_len
char     target[target_len]
<serialized payload>           // CommandPayload
```

### 5.2 Wire Format (server → dashboard)

### 5.2 wire format on network:

#### RegisterPayload

```
uint8_t     online
uint16_t    target_len
uint16_t    registered_at_len
uint16_t    last_seen_len
char        target[target_len]
char        registered_at[registered_at_len]
char        last_seen[last_seen_len]
<serialized payload>           // ResponsePayload or DataPayload
```

DATA / REGISRATION data =

```
uint16_t registration_count
RegisterPayload[registration_count]
```

#### DashboardCommand

Dashboard sends a DashboardCommand with sent_at field empty and commandPayload.id = 0, server will fill these fields. In case dashboard asks for a command history, server sends back a DashboardCommand witrh sent_at field filled

```
uint16_t      target_len
uint16_t      sent_at_len
char          target[target_len]
char          sent_at[sent_at_len]
<serialized   CommandPayload>  // command.id must be 0; server assigns the real id
```

#### DashboardResponse

```
uint16_t      target_len
uint16_t      received_at_len
char          target[target_len]
char          received_at[received_at_len]
<serialized   DashboardResponse>
```

#### DashboardDisconnect & DashboardData

The other Dashboard <-> server structures on the wire (DashboardDisconnect, DashboardData)

```
uint16_t      target_len
char          target[target_len]
<serialized payload>           // ResponsePayload or DataPayload
```

### 5.3 Structures

```cpp
// Agent registration info sent to dashboard
struct RegisterPayload {
    std::string    id;        // agent MAC address
    bool           online
    std::string    registered_at;
    std::string    last_seen;
    OsInfoPayload  system;
};

// Dashboard sends command to server OR server sends a command history (sent_at filled)
struct DashboardCommand {
  std::string target;
  CommandPayload command;
  std::string sent_at;
}

// Server forwards agent response to dashboard (one per chunk)
struct DashboardResponse {
    std::string     target;   // agent MAC address
    std::string received_at;
    ResponsePayload response; // chunk forwarded as-is; dashboard reassembles
};

// Server forwards agent data to dashboard
struct DashboardData {
    std::string target;      // agent MAC address
    DataPayload data;
};

// Dashboard ask for an agent to disconnect
struct DashboardDisconnect {
  std::string target;  // agent MAC address

};

```

### 5.4 Rules

- Agent and Dashboard **MUST** send a `REGISTER` or `DASHBOARD_REGISTER` containing a `RegisterPayload` as first message for identification at runtime.
- Server **MUST** reject any communication from unidentified source
- Dashboard **MUST** send COMMAND with `command.id = 0`; server replaces it with a database-generated id before forwarding to the agent.
- Server **MUST** track `commandId → agent target (MAC)` to route responses back correctly.
- Server **MUST** forward each RESPONSE chunk individually; dashboard **MUST** reassemble using `id`, `chunk_index`, `total_chunks`.
- Server **MUST** forward each `DATA / METRICS_SAMPLE` from agent as `DashboardData` to dashboard
- When a new agent registers, server **SHOULD** push a `DATA / REGISTRATION` frame to dashboard containing a single `RegisterPayload` through a `DashboardData`.
- When dashboard registers, server **SHOULD** send a `DATA / AGENTS` frame containing all currently connected agents as a list of `RegisterPayload` through a `DashboardData`.
- On any `DATA` type message, server **MUST** use `DashboardData` wrapper
- DISCONNECT from dashboard **MUST** carry a `DashboardDisconnect` payload; DISCONNECT from agent or server **SHOULD NOT** carry a payload.
- On a regular agent disconnection, server **MUST** send a DISCONNECT message using `DashboardDisconnect` with the agent id so dashboard can update GUI.
- On any error, server **MUST** inform sender and **MAY** inform dashboard

---

## 6. Stream Parsing

TCP delivers a continuous byte stream. Receivers **MUST** reconstruct frames:

1. Buffer all incoming bytes.
2. When buffer contains ≥ 8 bytes: parse the header, read `size`.
3. Compute `total_size = 8 + size`.
4. If `buffer.size() < total_size`: wait for more data.
5. Extract `total_size` bytes, dispatch the frame, repeat.

A single message may be split across multiple `recv()` calls. Bytes from different messages never interleave.

---

## 7. Versioning

| Agent version      | Server behavior              |
| ------------------ | ---------------------------- |
| < protocol version | Reject connection            |
| = protocol version | Accept                       |
| > protocol version | Accept (backward-compatible) |

Version is checked on REGISTER.

---

## 8. Limits

| Constraint            | Value                                       |
| --------------------- | ------------------------------------------- |
| Max payload size      | 65535 bytes                                 |
| Max chunk data        | 65509 bytes (65535 − 26 fixed (9 + 17 MAC)) |
| MAC address size      | 17 bytes (`"aa:bb:cc:dd:ee:ff"`)            |
| Max concurrent agents | Implementation-defined                      |

---

## 9. Notes

- All integers are big-endian.
- All strings are UTF-8, serialized as `uint16_t length + char[length]`, no null terminator.
- The agent MAC address is the persistent agent identity. The server uses it to recognize reconnecting agents and as the `target` field in all dashboard communication.
- COMMAND ids are server-generated and unique per server session. They map responses back to the originating command and agent.
