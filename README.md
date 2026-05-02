# Adaptive OS Memory Engine (CLI Version)

A sophisticated system monitor for Linux that performs real-time process analysis, hotness scoring, and simulated multi-layer storage distribution using advanced data structures.

## Features
- **Real-time Linux Process Scanning**: Uses \`/proc\` filesystem and \`sysinfo\`.
- **Advanced Data Structures**: Implementation of RB-Tree, Skip List, Max Heap, LRU Cache, Segment Tree, and Fenwick Tree for telemetry analytics.
- **Adaptive Scoring**: Processes are classified as **HOT**, **WARM**, or **COLD** based on CPU, Memory, Frequency, and Recency.
- **Priority Management**: Automatically boosts system priority (\`nice\`/\`setpriority\`) for "HOT" processes.
- **Pure Terminal UI**: Lightweight, interactive dashboard for monitoring system performance.

## Prerequisites
- C++17 Compiler (GCC/Clang)
- CMake 3.10+
- Linux (Ubuntu/Debian recommended)

## Build and Run
\`\`\`bash
mkdir build
cd build
cmake ..
make
sudo ./analyzer_cli
\`\`\`
*Note: \`sudo\` is required to collect advanced metrics and apply priority changes.*

## Project Structure
- **main_cli.cpp**: Entry point for the terminal dashboard.
- **analyzer.h**: Core logic and data structure coordination.
- **i_process_collector.h**: OS abstraction layer.
- **linux_process_collector.h**: Linux-specific implementation for process scanning.
- **data_structures.h**: Implementation of all ranking and scoring algorithms.
