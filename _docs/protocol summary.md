# ATFP Binary Protocol — Summary (Draft v0.1)

## 1. Overview

ATFP is a custom binary protocol designed for a **client-server architecture**:

* The **client connects** to the server
* The client **registers itself**
* The server sends **COMMANDS**
* The client executes them and returns **RESPONSES**
* The client may also send **unsolicited DATA**

The protocol is designed to be:

* Cross-platform
* Lightweight
* Extensible

---

## 2. Transport Layer

ATFP runs over **TCP**.

TCP provides:

* Reliable delivery
* Ordered messages
* Stream-based communication

---

## 3. Message Structure

Each message is composed of:

```id="hdrfmt"
+------------+---------+------+--------+
| Identifier | Version | Type | Size   |
| (4 bytes)  | (1 B)   | (1B) | (2 B)  |
+------------+---------+------+--------+
| Payload (variable, Size bytes)       |
+--------------------------------------+
```

### Header Fields

#### Identifier (4 bytes)

* Protocol identifier (ASCII)
* Example: `"ATFP"`

#### Version (1 byte)

* Current version: `0x01`

#### Type (1 byte)

| Value | Name       |
| ----- | ---------- |
| 0     | REGISTER   |
| 1     | DATA       |
| 2     | COMMAND    |
| 3     | RESPONSE   |
| 4     | DISCONNECT |
| 5     | ERROR      |

#### Size (2 bytes)

* Payload size in bytes
* Max value: `65535`
* Messages exceeding this MUST be rejected

---

## 4. Encoding Rules

* All integers are encoded in **big-endian (network byte order)**
* Variable-length fields MUST include a length prefix
* Strings are NOT null-terminated

---

## 5. Payload Definitions

### 5.1 REGISTER

Sent by client after connection.

```c++
struct RegisterPayload {
    uint8_t os_type;
    uint8_t arch;
    uint16_t hostname_length;
    char hostname[hostname_length];
};
```

#### OS Types

* 0 = Windows
* 1 = Linux
* 2 = macOS

#### Architectures

* 0 = x86
* 1 = x64
* 2 = ARM

#### Hostname

- Empty hostname not allowed
- Max hostname length = Max value (65535) - rest of payload

---

### 5.2 COMMAND

Sent by server to request an action.

```c++
struct CommandPayload {
    uint16_t id;
    uint8_t type;
    uint16_t data_length;
    char data[data_length];
};
```

#### Command Types

* 0 = OS_INFO
* 1 = RUNNING_PROCESSES
* 2 = SHELL

Notes:

* OS_INFO → no data
* RUNNING_PROCESSES → no data
* SHELL → data contains command string

---

### 5.3 RESPONSE

Sent by client after executing a command.

```c++
struct ResponsePayload {
    uint16_t id;
    uint8_t type;
    uint8_t status;
    uint16_t data_length;
    char data[data_length];
};
```

#### Status

* 0 = OK
* 1 = ERROR

---

### 5.4 DATA

Unsolicited client messages.

```c++
struct DataPayload {
    uint8_t subtype;
    uint16_t data_length;
    char data[data_length];
};
```

Examples:

* Keylogger stream
* Periodic system info

---

### 5.5 ERROR

```c++
struct ErrorPayload {
    uint8_t code;
    uint16_t message_length;
    char message[message_length];
};
```

#### Error Codes

* 0 = UNKNOWN_TYPE
* 1 = INVALID_FORMAT
* 2 = UNKNOWN_COMMAND
* 3 = EXECUTION_FAILED

---

### 5.6 DISCONNECT

* No payload

---

## 6. Communication Flow

### 6.1 Connection

1. Client connects via TCP
2. Client sends REGISTER

---

### 6.2 Command Execution

1. Server sends COMMAND
2. Client executes command
3. Client sends RESPONSE (same id)

---

## 7. TCP Stream Handling

TCP is stream-based, not message-based.

Implementations MUST:

1. Read exactly 8 bytes (header)
2. Extract payload size
3. Read exactly `size` bytes (payload)

Partial reads MUST be handled correctly.

---

## 8. Error Handling

The implementation MUST:

* Reject invalid message formats
* Reject unknown message types
* Reject payloads exceeding size limit

The implementation SHOULD:

* Send ERROR messages when possible

---

## 9. Checksum (Optional)

A checksum MAY be added:

```id="checksumfmt"
[HEADER][PAYLOAD][CRC32 (4 bytes)]
```

* CRC32 computed over header + payload
* If used, MUST be validated before processing

---

## 10. Extensibility

* New command types MAY be added
* Unknown fields MUST be ignored
* Version MUST be checked

---
