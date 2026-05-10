# Circle 01

## What to anticipate

1. Cross-Platform Sockets (Anticipating Cercle 07)

   - **What's coming:** The agent must eventually run on both Linux and Windows. Classes encapsulating syscalls will need to inherit from an interface depending on the OS.
   - **What to do now:** Design LPTF_Socket with cross-platform compatibility in mind. You will eventually have to handle POSIX sockets (Linux) and Winsock (Windows). Prepare to use preprocessor directives (#ifdef_WIN32, #ifdef **linux**) or setup an abstract ISocket interface right away.

2. Binary-Safe Data Buffering (Anticipating Cercle 02)

    - What's coming: You will implement a custom binary protocol using structures (LPTF_Packet), not strings.
    - What to do now: Do not rely on null-terminated strings (\0) or newline characters (\n) to detect the end of a message. TCP is a stream protocol; a single recv() might yield only half a packet. Your server/agent reactor should maintain a byte buffer (std::vector<uint8_t>) for each connection, accumulating raw bytes until enough are present to form a complete packet.
    -
3. Integration with a GUI & Threads (Anticipating Cercle 04 & 05)

    - **What's coming:** The server will receive a Qt GUI (Cercle 04) and a separate background thread for PostgreSQL database operations (Cercle 05, though hinted in the Cercle 01 notes).
    - **What to do now:** Since you cannot use threads for networking, you must use a non-blocking multiplexing syscall (like select, poll, or epoll). Keep this Reactor loop extremely clean and decoupled from your application logic. This ensures that when you integrate Qt, your socket polling won't block the UI thread, or it can be easily bridged with Qt's event system.

4. Graceful Disconnection & Lifecycle Management (Anticipating Cercle 04 & 08)

    - **What's coming:** The server UI will need a way to kick specific agents (Cercle 04), and the agent will need to silently attempt reconnections when dropped (Cercle 08).
    - **What to do now:** Ensure your network multiplexer accurately detects agent disconnections (e.g., when recv returns 0 or socket errors occur). LPTF_Socket should expose clear states (Connected, Disconnected, Error) so the agent program can loop a reconnection attempt cleanly rather than crashing.

5. Strict Constraints Check (Prerequisites)

    Because every function is limited to 25 lines and 85 columns, your LPTF_Socket and multiplexing logic (the select() or poll() loop) will likely need to be aggressively broken down into small, single-responsibility helper methods right from the start. Build your reactor as a class that maps file descriptors to callback functions or handler objects to keep the core loop minimal.

## LPTF_Socket

The LPTF_Socket class should absolutely be shared between the agent and the server. You should only write it once.

Here is why:

DRY Principle (Don't Repeat Yourself): The underlying system calls for networking (socket, bind, listen, accept, connect, send, recv, close) are identical whether you are building a agent or a server. Having two different classes would mean maintaining duplicate code.
The Instructions: The instructions explicitly say "Ces programmes devront être construits en s’appuyant sur une class LPTF_Socket" (singular).
Future-proofing (Cercle 07): In Cercle 07, you will have to create OS-specific implementations (e.g., LPTF_Socket_Windows and LPTF_Socket_Linux) inheriting from an interface. If you separate agent and server sockets now, you will eventually have to maintain 4 different classes instead of just 2.
How to structure it:

You should probably create a shared folder in your workspace (like common/ or network/) alongside agent, server, and protocol.

The LPTF_Socket class will contain all the generic methods wrapping the syscalls. Then:

Your server program will use the bind(), listen(), and accept() wrappers of LPTF_Socket.
Your agent program will use the connect() wrapper of LPTF_Socket.
Both will use the send() and recv() wrappers.
