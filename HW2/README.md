# Happy Friends Tree

## Description

The **Happy Friends Tree** project models a dynamic tree structure representing social connections. It allows for adding, checking, modifying, and terminating relationships within the tree, simulating real-world social interactions.

## Features

1. **Meet**: Add a node to the tree structure.
2. **Check**: Output the status of a subtree.
3. **Graduate**: Terminate all processes of a subtree.
4. **Adopt**: Modify the structure by moving a subtree to another node.

## Implementation Details

- **Meet**: Utilizes `pipe()`, `fork()`, `dup()`, and `exec()` to manage process creation and communication.
- **Check**: Designs I/O through pipes between parent and child processes to inspect the tree.
- **Graduate**: Handles subtree process elimination and resource cleanup.
- **Adopt**: Combines the above functionalities, addressing use cases and limitations of FIFO.

## Getting Started

### Prerequisites

- C compiler (e.g., `gcc`)
- Make sure to update your local files with the latest updates provided [here](https://drive.google.com/file/d/1oPqE41VsZO8zwQanzdYhwvoION6MPZRy/view?usp=drive_link).

### Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/your-username/Happy-Friends-Tree.git
   ```
2. Navigate to the project directory:
   ```bash
   cd Happy-Friends-Tree
   ```
4. Compile the program:
   ```bash
   gcc friend.c -o friend
   ```

### Usage
Run the program using:
```bash
./friend
```
