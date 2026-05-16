# The tools, explained
## GDB (GNU Debugger) 
lets you pause a running program, inspect what's in memory, step through code line by line, and see exactly where and why it crashed. Instead of adding printf everywhere to understand a bug, you set a breakpoint and interrogate the live process. Essential for any C++ project.
## Valgrind
runs your binary inside a virtual CPU that watches every memory operation. It tells you about memory leaks (allocated but never freed), use-after-free bugs (reading memory you already freed), and reads of uninitialised memory. C++ without a memory checker is dangerous; Valgrind is your safety net. It makes programs run ~20x slower, but you only use it during testing.
## file 
a tiny command that reads a binary and tells you what it actually is: ELF 64-bit LSB executable, x86-64 for example. Useful to quickly confirm you compiled the right architecture, or that a file isn't corrupted.
## strace
intercepts and prints every system call your program makes in real time. When your socket code hangs silently and you don't know why, strace ./server shows you exactly which recv() or connect() call is blocking, and what the kernel returned. Very useful for network code specifically.