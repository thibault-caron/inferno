# Architecture

## Proposed TDD - DDD structure (too atomised?)

```text
inferno/
├── apps/
│   ├── server/                 # Qt app bootstrap + composition root
│   └── client/                 # agent bootstrap + composition root
├── contexts/
│   ├── connection/             # sessions, client registry, routing
│   ├── protocol/               # packet model, serializer, versioning
│   ├── command-exec/           # authorized remote command use-cases
│   ├── host-info/              # host metadata collection use-cases
│   ├── process-list/           # process inventory use-cases
│   ├── analysis/               # data analysis domain rules
│   └── persistence/            # storage domain rules
├── shared/
│   ├── kernel/                 # Result, Error, ValueObject base, Clock, IDs
│   ├── messaging/              # events/commands interfaces
│   └── testing/                # common test fakes/builders
├── infrastructure/
│   ├── network/                # LPTF_Socket impl (TCP, reactor)
│   ├── database/               # LPTF_Database impl (PostgreSQL)
│   ├── os/
│   │   ├── windows/
│   │   └── linux/
│   └── gui/                    # Qt adapters/widgets
├── tests/
│   ├── unit/                   # domain + application services (fast)
│   ├── integration/            # DB/network adapters
│   ├── contract/               # client<->server protocol compatibility
│   └── e2e/                    # full scenarios
├── docs/
│   ├── rfc/                    # binary protocol RFC docs
│   └── adr/                    # architecture decisions
├── cmake/
└── CMakeLists.txt
```

## scalable cercle 01-02 architecture

### 1. Directory Structure

```text
inferno/
├── common/                   # Shared between client and server (Circle 07 prep)
│   ├── include/
│   │   ├── ISocket.hpp       # OS-agnostic Interface (Circle 07 prep)
│   │   ├── LPTF_Socket.hpp
│   │   └── LPTF_Packet.hpp
│   └── src/
│       ├── LPTF_Socket.cpp
│       └── LPTF_Packet.cpp
├── server/
│   ├── include/
│   │   ├── Server.hpp        # Holds the Reactor loop
│   │   └── ClientSession.hpp # Buffers data for a single client
│   └── src/
│       ├── main.cpp
│       ├── Server.cpp
│       └── ClientSession.cpp
└── client/
    ├── include/
    │   └── Client.hpp
    └── src/
        ├── main.cpp
        └── Client.cpp
```

anticipates the strict constraints and future requirements (like the Qt GUI and binary protocol) of the later circles.

Create a strict separation of concerns early on to make building the final executables easy.

### 2. Core Components (Circle 01: Networking)

1. The Interface First (ISocket)

    To anticipate Circle 07, define an ISocket interface. LPTF_Socket will inherit from this. When you reach Circle 07, you can easily split this into LPTF_Socket_Linux and LPTF_Socket_Windows.

2. The Network Multiplexer: The "Reactor" Pattern

    Since you cannot use threads or forks for networking, your server must use non-blocking I/O with select() or poll().

   - Create a Server class that maintains a list of connected clients.
   - The Server::run() method contains an infinite loop that calls poll().
   - When integrating Qt in Circle 04, you will simply hook this non-blocking poll() step into a QTimer or Qt's event loop, keeping the GUI perfectly responsive without needing threads for the network.

3. The ClientSession Wrapper

TCP is a stream, not a packet system. A single recv() might yield half a message, or two messages glued together.
Wrap each LPTF_Socket in a ClientSession class on the server side. It should contain:

A reference to the LPTF_Socket.
An incoming buffer (std::vector<uint8_t> rx_buffer).
An outgoing buffer (std::vector<uint8_t> tx_buffer).
When poll() says a socket is ready to read, append the bytes to rx_buffer, then check if a full LPTF_Packet can be extracted.

1. The Protocol Architecture (Circle 02: LPTF_Packet)
To support Circle 03 (keylogger, process lists, OS specs), your custom binary protocol needs a fixed-size header and a variable-length payload.

The Header Structure (Fixed Size)

```c++
#pragma pack(push, 1) // Ensure no padding bytes are added by the compiler
struct PacketHeader {
    uint8_t  magic[2];    // E.g., 'L', 'P' to verify it's our protocol
    uint16_t command_id;  // E.g., 0x01 = GetOS, 0x02 = KeyloggerToggle
    uint32_t payload_len; // How many bytes follow the header
};
#pragma pack(pop)
```

The LPTF_Packet Class
Your LPTF_Packet class should manage the header and the raw payload bytes.

void serialize(std::vector<uint8_t>& out_buffer) const;
bool parse(const std::vector<uint8_t>& in_buffer);
How it scales:
When the server wants the client's process list (Circle 03), it constructs an LPTF_Packet with command_id = 0x05, empty payload, and serializes it. The client receives it, reads the command ID, executes the local system call, packs the result into a new payload, sets command_id = 0x06 (Response), and sends it back.

4. Handling the 25-line Limit
The strict Coplien form and 25-line limit mean your code needs to be highly modular.
Instead of a massive switch(command_id) inside your server to handle incoming packets, use a Command Pattern or a std::map of function pointers/handlers.

```c++
// Inside Server or a PacketDispatcher class
using CommandHandler = void (*)(ClientSession&, const LPTF_Packet&);
std::map<uint16_t, CommandHandler> handlers;

// Register them early:
handlers[CMD_OS_INFO_RESP] = &handleOSInfoResponse;
handlers[CMD_KEYLOG_DATA]  = &handleKeylogData;
```

This keeps your networking loop clean: extract packet -> lookup handler in map -> execute -> back to poll().