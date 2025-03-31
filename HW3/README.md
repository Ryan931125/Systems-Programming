# User-Level Thread Library

## Overview

This project involves the development of a **User-Level Thread Library** that enables multithreading capabilities within a single process. The library is designed to manage multiple threads, allowing for concurrent execution and efficient CPU utilization without relying on kernel-level threading.

## Features

1. **Thread Creation and Management**: Implement functions to create, initialize, and manage threads within the user space.
2. **Context Switching**: Develop mechanisms to switch between different thread contexts, facilitating cooperative multitasking.
3. **Synchronization**: Implement synchronization primitives to manage access to shared resources among threads.
4. **Signal Handling**: Incorporate signal handling to manage thread interruptions and ensure robust execution.

## Implementation Details

- **Thread Control Block (TCB)**: Design a `struct tcb` to maintain thread-specific information such as thread ID, state, stack pointer, and other necessary metadata.
- **Scheduler**: Implement a scheduler to manage thread execution order based on a predefined scheduling policy.
- **Context Switching Mechanism**: Utilize functions like `setjmp()` and `longjmp()` to save and restore thread contexts, enabling seamless switching between threads.
- **Synchronization Primitives**: Develop mutexes or semaphores to prevent race conditions and ensure data consistency when multiple threads access shared resources.
- **Signal Handling**: Implement signal handlers to catch and manage signals like `SIGTSTP`, ensuring graceful suspension and resumption of threads.
