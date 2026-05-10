# How Sockets Work — From Scratch

---

## What a socket actually is

A socket is just a **file descriptor** — an integer the OS gives you that represents one end of a connection. When you create one:

```cpp
int fd = ::socket(AF_INET, SOCK_STREAM, 0);
```

The OS creates two internal buffers attached to that fd:

```
Your program
    │
    │  fd = 7
    │
┌───▼────────────────────────┐
│         OS kernel          │
│                            │
│  receive buffer [........] │  ← incoming data lands here
│  send buffer    [........] │  ← outgoing data waits here
└────────────────────────────┘
    │
  network
```

You never touch the network directly. You talk to those buffers. The OS handles the rest.

---

## The server side — binding and listening

```cpp
::bind(fd, &addr, sizeof(addr));   // "this socket belongs to port 4242"
::listen(fd, 10);                  // "start accepting connections, queue up to 10 pending ones"
```

After `listen()`, your server socket isn't for sending or receiving data.
It is purely a **doorbell** — its only job is to notify you when someone wants to connect.

The `10` in `listen()` is the **backlog** — how many incoming connections the OS will queue
before it starts refusing new ones. If your server is busy and 10 agents knock at the same time,
the OS holds them in a waiting room. The 11th gets rejected.

---

## A client connects — what actually happens

```
Agent machine                         Server machine
─────────────                         ──────────────
::connect(serverIp, 4242)  ──────►   OS sees incoming connection
                                      adds it to the listen queue
                           ◄──────   OS sends back TCP handshake (SYN/ACK)
                                      (you didn't do this, the OS did automatically)
```

The TCP handshake (SYN, SYN-ACK, ACK) is handled entirely by the OS — you never see it.
By the time you call `accept()`, the connection is already fully established.

```cpp
int agentFd = ::accept(serverFd, &agentAddr, &addrLen);
```

`accept()` pulls one connection out of the waiting room and gives you a **brand new fd**
for that specific agent. Now you have two fds:

```
serverFd  →  still the doorbell, waiting for more connections
agentFd   →  a private line to this specific agent, with its own buffers
```

---

## Sending data

```cpp
::send(agentFd, data, length, 0);
```

This copies your data into the **send buffer** of that socket. The OS then takes care of
putting it on the wire, handling retransmits if packets get lost, and so on.

`send()` returns as soon as the data is in the buffer — not when the other side actually received it.

```
your code → send buffer → [OS handles network] → agent's receive buffer → agent's recv()
```

If the send buffer is full (the agent is slow to read), `send()` will block —
or return `EWOULDBLOCK` if the socket is set to non-blocking mode.

---

## Receiving data

```cpp
::recv(agentFd, buf, sizeof(buf), 0);
```

This copies data **out of** the receive buffer into your `buf`. If the buffer is empty:

- **Blocking socket** → your program freezes until data arrives
- **Non-blocking socket** → returns `-1`, `errno = EWOULDBLOCK` ("nothing here right now, come back later")

If there is data, it returns the number of bytes copied.

> **Important:** `recv()` does not guarantee it copied everything.
> If 500 bytes arrived and your buffer is 200 bytes, you get 200.
> The remaining 300 stay in the kernel buffer waiting for your next `recv()` call.
> This is why you typically accumulate chunks into your own buffer until you have a complete message.

---

## The connection closes

When the other side disconnects, `recv()` returns `0`. Not `-1`, not an error — exactly `0`.

```cpp
ssize_t n = ::recv(agentFd, buf, sizeof(buf), 0);
if (n == 0)   // peer disconnected cleanly
if (n == -1)  // actual error (check errno)
if (n > 0)    // data received, n = number of bytes
```

---

## Blocking vs non-blocking sockets

By default a socket is **blocking**:
- `recv()` with no data → program freezes and waits
- `send()` with a full buffer → program freezes and waits

When you set a socket to **non-blocking**:
- `recv()` with no data → returns `-1` instantly, `errno = EAGAIN / EWOULDBLOCK`
- `send()` with a full buffer → returns `-1` instantly, `errno = EAGAIN / EWOULDBLOCK`

`EAGAIN` and `EWOULDBLOCK` mean the same thing on Linux. Historically they were separate values,
so defensive code handles both.

---

## The full picture — one complete trace

```
Server starts
│
├─ ::socket()     → serverFd created
├─ ::bind()       → serverFd attached to port 4242
├─ ::listen()     → OS starts accepting knock on port 4242
│
Agent connects
│
├─ ::accept()     → agentFd created (new private line to this agent)
│
Agent sends "hello"
│
├─ OS puts "hello" in agentFd's receive buffer
├─ ::recv()       → copies "hello" into your buf, returns 5
│
You reply
│
├─ ::send()       → copies your reply into agentFd's send buffer
├─ OS puts it on the wire
│
Agent disconnects
│
├─ ::recv()       → returns 0
├─ ::close(agentFd) → release the fd, free the buffers
```

---

## Quick reference

| Call | Who uses it | What it does |
|---|---|---|
| `socket()` | both | creates an fd with send/receive buffers |
| `bind()` | server | attaches the fd to a port |
| `listen()` | server | starts accepting incoming connections |
| `accept()` | server | pulls one pending connection, returns a new fd |
| `connect()` | client | initiates a connection to a server |
| `send()` | both | copies data into the send buffer |
| `recv()` | both | copies data out of the receive buffer |
| `close()` | both | releases the fd and its buffers |