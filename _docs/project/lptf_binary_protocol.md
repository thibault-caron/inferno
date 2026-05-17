# lptf binary protocol

## 1. Overview

LPTF is a custom binary agent-server protocol:

- Agent connects and registers itself.
- Server sends COMMANDS.
- Agent executes commands and sends RESPONSES (possibly chunked).
- Agent may send unsolicited DATA.
- TCP sockets are used as the transport.

The protocol is designed to be:

- Cross-platform
- Lightweight
- Extendable

## 2. Transport

- Use TCP sockets (SOCK_STREAM) at low level.
- TCP provides reliability, ordered delivery, and byte-stream semantics.
- Your protocol is responsible for parsing bytes into messages, including chunked responses.

## 3. Message Format

All messages have a fixed-size header followed by a payload:

```text
[Identifier][Version][Type][Size] 
 4 bytes     1B      1B     2B
```

| Field      | Size | Description                          |
| ---------- | ---- | ------------------------------------ |
| Identifier | 4B   | ASCII string (e.g. `"LPTF"`)         |
| Version    | 1B   | Protocol version (current: `1`)      |
| Type       | 1B   | Message type                         |
| Size       | 2B   | Payload length (uint16, max `65535`) |

Payload immediately follows the header.

### 3.1 Message Types

| Value | Name       | Description                  |
| ----- | ---------- | ---------------------------- |
| 0     | REGISTER   | Agent → Server registration |
| 1     | DATA       | Unsolicited agent data      |
| 2     | COMMAND    | Server → Agent instruction  |
| 3     | RESPONSE   | Agent → Server result       |
| 4     | DISCONNECT | Close connection             |
| 5     | ERROR      | Error reporting              |

## 4. Payload Structures

### 4.1 REGISTER

Agent sends REGISTER immediately after connecting.

```c++
struct RegisterPayload {
    uint8_t os_type;        // 0=Windows, 1=Linux, 2=macOS
    uint8_t arch;           // 0=x86, 1=x64, 2=ARM
    uint16_t hostname_len;
    char hostname[hostname_len]; // UTF-8
};
```

Rules:

- Server ignores other messages until REGISTER is received.
- Invalid or missing REGISTER → connection may be closed.

### 4.2 COMMAND

Server sends COMMAND to request action:

```c++
struct CommandPayload {
    uint16_t id;        // unique id for this command
    uint8_t type;       // OS_INFO, RUNNING_PROCESSES, SHELL
    uint16_t data_len;  // optional command string length (SHELL only)
    char data[data_len];
};
```

#### Command types

| Value | Name              |
| ----- | ----------------- |
| 0     | OS_INFO           |
| 1     | RUNNING_PROCESSES |
| 2     | SHELL             |
| 3     | START_KEYLOGGER   |
| 4     | STOP_KEYLOGGER    |

### 4.3 RESPONSE

Agent sends RESPONSE after executing a command. Supports chunking:

```c++
struct ResponsePayload {
    uint16_t id;          // command id this response belongs to
    uint8_t status;       // 0=OK, 1=ERROR
    uint8_t total_chunks; // total number of chunks
    uint8_t chunk_index;  // 0-based index of this chunk
    uint16_t data_len;    // length of this chunk
    char data[data_len];  
};
```

#### Chunking rules

- Each chunk ≤ 65535 bytes
- Server reassembles using:
  - `id`
  - `chunk_index`
  - `total_chunks`
- Large outputs must be split into multiple chunks

### 4.4 DATA

For unsolicited agent data:

```c++
struct DataPayload {
    uint8_t subtype;        // custom type
    uint16_t data_len;
    char data[data_len];
};
```

### 4.5 ERROR

```c++
struct ErrorPayload {
    uint8_t code;           // see table below
    uint16_t message_len;
    char message[message_len]; // UTF-8
};
```

#### Error codes

| Code | Meaning          |
| ---- | ---------------- |
| 0    | UNKNOWN_TYPE     |
| 1    | INVALID_FORMAT   |
| 2    | UNKNOWN_COMMAND  |
| 3    | EXECUTION_FAILED |
| 4    | SIZE_EXCEEDED    |

### 4.6 DISCONNECT

- No payload
- Server may force disconnect
- Agent must:
  - Close socket
  - Optionally reconnect

## 5. Parsing Guidelines (TCP Stream)

TCP provides a continuous byte stream. You must reconstruct messages:

- Maintain a receive buffer.
- Append all received bytes.
- While buffer contains at least 8 bytes (header):
  - Peek header → read size
  - Compute:  
        `total_size = 8 + size`
  - If buffer < total_size → wait for more data
  - Else:
    - extract full message
    - process it
    - Repeat (handle multiple messages in the buffer)

Important: bytes from different messages never interleave, but a single message may be fragmented across multiple recv() calls.

## 6. Versioning

- Agent version < protocol version → reject
- Agent version > protocol version → accept (backward-compatible)
- Always check version in REGISTER

## 7. Limits & Timeouts

- Max payload = 65535 bytes (chunking for larger data)
- Optional: limit number of concurrent agents

### Optional constraints

- Limit number of concurrent agents
- Disconnect if message incomplete after timeout

## 8. Notes

- All integers are big-endian
- All strings are UTF-8
- COMMAND id ensures correct mapping of RESPONSES, even with multiple simultaneous commands
- Chunking ensures no single message exceeds the max payload size
