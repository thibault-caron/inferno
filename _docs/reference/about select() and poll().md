# About select() & poll()

## Q

"Since you cannot use threads for networking, you must use a non-blocking multiplexing syscall (like select, poll, or epoll)"

since the server will "send" request and the agent will send reponse, or "data" (without request, from the server), wouldn't the efficient syscall be "push" rather than select or poll?

## A

That is a very common confusion between high-level architectural concepts and low-level system calls!

Here is why you still absolutely need `select`, `poll`, or `epoll`, and why "push" isn't what you are looking for:

1. There is no `push` syscall

    In standard OS networking (POSIX or Winsock), there is no system call named `push`.  

    When you want to "push" data to a agent, you use the `send()` or `write()` syscalls. When you want to pull data, you use `recv()` or `read()`.

2. `select` and `poll` are not for sending data; they are for *monitoring*

    If your server is connected to 10 agents simultaneously, how do you know which agent just sent a response?  

    If you simply loop through them and call `recv(agent_1)`, your entire program will **freeze (block)** waiting for Agent 1 to say something. If Agent 2 sends important data while your program is frozen waiting on Agent 1, your server misses it.

Because you are banned from using threads in Cercle 01, you cannot give each agent its own thread to wait on `recv()`.

The role of Multiplexing (select / poll / epoll)
These syscalls act like a radar. You give select() a list of all your connected sockets, and it pauses your program until at least one of them does something.

It returns and tells you:

- "Agent 3 just sent data, you can safely call recv() on it without freezing!"
- "A new agent is trying to connect, you can safely call accept()!"
- "Agent 5 disconnected."

### Summary

You still use `send()` to push your requests to the agents.
You use select or poll to monitor all agents simultaneously on a single thread so you know exactly when and who to call `recv()` on without freezing your server.

## Q

define multiplexing in the context of using reactor to avoid multithreading, maybe compared with multiplexing for http2

## A

The word **multiplexing** broadly means "handling multiple things using a single shared resource." However, depending on the context, the "things" and the "shared resource" are completely different.

Here is the breakdown of what multiplexing means in your C++ Reactor (OS level) versus HTTP/2 (Protocol level).

---

### 1. I/O Multiplexing (Your Server / The Reactor Pattern)

**The shared resource:** A single CPU Thread.  
**The things being managed:** Multiple TCP connections (sockets from different agents).

In older or simpler architectures, a server creates a new thread for every agent that connects. If agent A takes 10 minutes to send a message, Thread A just sits there frozen (blocked) on recv().

To avoid threads (as forbidden by your instructions), you use I/O **Multiplexing** via system calls like `select`, `poll`, or `epoll` inside a Reactor loop.

- Instead of waiting on one agent, you hand the OS an array of all 50 connected sockets.
- You say: "Pause my single thread until ANY of these 50 sockets has data ready to read, or is ready to be written to without blocking."
- The OS wakes up your thread and says: "Sockets #4 and #12 have incoming data."
- Your single thread quickly calls recv() on #4, then recv() on #12, processes the data, and loops back to ask the OS to wait again.

**Analogy:** A waiter in a restaurant (one thread) doesn't stand at Table 1 waiting 10 minutes for them to decide what to order (blocking). Instead, the waiter stands in the middle of the room and watches all tables simultaneously (multiplexing). When Table 4 raises their hand (event ready), the waiter goes there.

### 2. Protocol Multiplexing (e.g., HTTP/2)

**The shared resource:** A single TCP Connection (socket).  
**The things being managed:** Multiple logical data requests (e.g., HTML, CSS, Image).

In HTTP/1.1, if a browser wanted 3 images from a server, it either had to open 3 separate TCP connections, or send Request 1, wait for Response 1 to finish completely, then send Request 2 (Head-of-line blocking).

**HTTP/2 Multiplexing** solves this at the application layer. It chops the data into tiny chunks called "Frames" and tags each frame with a Stream ID. It then shuffles them all over **one single TCP connection.**

[Frame: Image A (part 1)]
[Frame: CSS (part 1)]
[Frame: Image A (part 2)]
[Frame: JS (part 1)]
Analogy: A single literal pipe (the TCP socket) where you are pouring red, blue, and green marbles down it simultaneously. At the other end, the receiver sorts them by color to rebuild the original files.

### Summary Comparison

| Concept               | What is shared? | What is being multiplexed? | How is it done?    | Who does the work?   |
|-----------------------|-----------------|----------------------------|--------------------|----------------------|
| Reactor (select/poll) | 1 CPU Thread    | Many TCP Sockets           | Event polling loop | The Operating System |
| HTTP/2                | 1 TCP Socket    | Many HTTP Requests         | Frame interleaving | The Application Code |

For **Cercle 01**, you only care about the first one. You are multiplexing your thread using `poll()` to juggle multiple agents safely.

Later, in **Cercle 02**, when you design your binary protocol structures, you could technically add a "Session ID" to your packet header if you wanted to multiplex multiple simultaneous file transfers over a single agent connection—but for a simple RAT, that is usually overkill!
