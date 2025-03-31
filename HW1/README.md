# csieTrain System

## Overview

The **csieTrain** system is designed to enhance the train seat reservation process by efficiently handling multiple client requests simultaneously. It comprises two main components:

- **Read Server**: Allows passengers to check seat availability.
- **Write Server**: Manages seat reservations and updates seat records.

## Features

1. **Concurrent Request Handling**: Utilizes multiplexing techniques (`select()` or `poll()`) to manage multiple client connections without blocking.
2. **Record Consistency**: Implements file locking mechanisms to prevent simultaneous modifications and ensure data integrity.
3. **Connection Timeout Management**: Automatically closes idle connections after a predefined period to free up resources and prevent blocked reservations.
4. **Robust Connection Handling**: Manages connection disruptions gracefully, ensuring incomplete messages do not impact other client requests.

## Implementation Details

- **Multiplexing**: Employed `select()` system call to monitor multiple file descriptors, enabling the server to handle multiple client connections efficiently.
- **File Locking**: Used `flock()` to implement advisory locks on seat map files, ensuring that only one process can modify a file at a time.
- **Timeout Mechanism**: Integrated a timeout feature that tracks client activity and disconnects inactive clients after a specified duration.
- **Signal Handling**: Implemented signal handlers to manage unexpected client disconnections and clean up resources appropriately.

## Getting Started

### Prerequisites

- **Operating System**: Unix-like system (e.g., Linux)
- **Compiler**: GCC
- **Development Tools**: Make

### Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/your-username/csieTrain.git
   ```
2. **Navigate to the Project Directory**:
   ```bash
   cd csieTrain
   ```
3. **Compile the Servers**:
   - To compile the read server:
     ```bash
     gcc server.c -D READ_SERVER -o read_server
     ```
   - To compile the write server:
     ```bash
     gcc server.c -D WRITE_SERVER -o write_server
     ```
   
### Usage

1. **Start the Write Server**:
   ```bash
   ./write_server
   ```
3. **Start the Read Server**:
   ```bash
   ./read_server
   ```
5. **Client Interaction**:
   - Use a TCP client (e.g., telnet or a custom client application) to connect to the servers.
   - Follow on-screen prompts to check seat availability or make reservations.
