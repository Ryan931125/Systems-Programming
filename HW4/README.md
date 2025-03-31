# Memory Management Unit Simulation

## Overview

This project involves developing a **Memory Management Unit (MMU) Simulation** that emulates the behavior of a computer's memory management system. The simulation includes functionalities such as address translation, page replacement, and handling page faults, providing insights into how operating systems manage memory resources.

## Features

1. **Address Translation**: Converts logical addresses to physical addresses using a page table mechanism.
2. **Page Replacement Algorithms**: Implements various algorithms (e.g., FIFO, LRU) to manage page replacements when the memory is full.
3. **Page Fault Handling**: Detects and handles page faults by loading the required pages into memory.
4. **TLB Simulation**: Simulates a Translation Lookaside Buffer to cache recent address translations for faster access.

## Implementation Details

- **Page Table Structure**: Designs a page table to map logical addresses to physical frames, including valid/invalid bits and frame numbers.
- **TLB Implementation**: Implements a cache that stores recent translations to reduce the time needed for address resolution.
- **Page Replacement Policies**: Provides options for different page replacement strategies, allowing comparison of their efficiencies.
- **Simulation Input**: Reads a sequence of memory access requests from an input file to simulate real-world scenarios.

