# Adaptive OS Memory Engine (v1.0.0 Stable)

A high-performance, real-time memory management and process optimization engine built for Linux. This engine implements a sophisticated tiered storage model (L1/L2/L3) to dynamically reallocate system resources based on process behavior, ensuring that mission-critical tasks always have the lowest latency access to RAM.

---

## Full Implementation Overview

The engine operates as a continuous monitor that intercepts system telemetry, scores every running process using a weighted algorithm, and then physically reorganizes how the OS treats those processes.

### 1. Telemetry Collection (Linux Native)
While the Windows version relies on the Windows API (WMI and Toolhelp32), the Linux version hooks directly into the kernel's virtual filesystem and POSIX standards:
- **Process Data**: Reads from /proc/[pid]/stat for CPU timing and /proc/[pid]/status for physical memory (RSS).
- **Process Control**: Uses the 'kill' system call to send SIGSTOP and SIGCONT signals for non-destructive process freezing.
- **Priority Management**: Uses 'setpriority' to adjust process nice values (-20 to 19), a direct alternative to the Windows SetPriorityClass API.
- **Terminal IO**: Uses termios.h and fcntl.h for raw keyboard input handling in the CLI.

### 2. Weighted Hotness Algorithm
Every process is assigned a Hotness Score (0-100) calculated across five dimensions:
- **Frequency (20%)**: How often the process is accessed/focused.
- **Recency (25%)**: How long since the process was last active (LRU logic).
- **Active Time (20%)**: Total uptime vs. idle time.
- **Memory (15%)**: Physical footprint (detects "Memory Hogs").
- **CPU (20%)**: Real-time processing demand.

---

## Advanced Data Structures (The Engine Core)

The engine uses a hybrid architecture of data structures to achieve O(1) and O(log n) performance:
- **Red-Black Tree & Skip List**: Maintains a perfectly sorted ranking of all system processes at all times.
- **Max-Heap (Pairing Heap)**: Used for instantaneous "Top-K" extraction and eviction of the "coldest" processes.
- **Fenwick & Segment Trees**: Power the "Frequency Analysis" and "Time-Series" dashboards, allowing for range queries on process activity.
- **LRU Cache (Doubly Linked List + Hash Map)**: Tracks the precise temporal order of process usage.
- **Process Hash Map**: Our primary storage for O(1) process data retrieval by PID.

---

## Linux vs. Windows 11 Version

The Linux implementation is a significant architectural departure from the Windows reference:

| Feature | Windows 11 Version (Qt GUI) | Linux CLI Version (High Performance) |
| :--- | :--- | :--- |
| **Interface** | Qt Widgets / GraphicsView | ANSI Terminal Dashboard |
| **Telemetry API** | WMI / Toolhelp32 Snapshot | /proc Virtual Filesystem |
| **Process Control** | TerminateProcess / SuspendThread | SIGSTOP (Freeze) and SIGCONT (Resume) |
| **Scheduling** | SetPriorityClass / SetThreadPriority | setpriority() (Nice values) |
| **Memory Model** | Virtual Memory HUD | Resident Set Size (RSS) Telemetry |
| **Tiering** | Simulated UI Layers | Functional Tier-Sync with OS Priority |
| **Overhead** | High (UI Rendering) | Near-Zero (<0.1% CPU) |

---

## Memory Waste Management

The engine identifies processes that are classified as COLD but occupy >100MB of RAM.
- **Demotion**: Automatically moved to L3 DISK (simulated high-latency swap).
- **Supression**: Assigned the lowest execution priority to prevent cache-line pollution.

---

## Building and Running

### Prerequisites
- **Compiler**: g++ (C++17 or higher)
- **Build System**: cmake (3.10+)
- **OS**: Linux (Tested on Ubuntu/Debian/Arch)

### Setup
```bash
# Clone the repository
git clone https://github.com/shivambhadane729/-Multi-Layer-Storage-Engine.git
cd adaptive-os-memory-engine

# Build
rm -rf build && mkdir build 
cmake -S . -B build
cmake --build build
```

### Execution
The engine requires Root Privileges to manipulate process priorities and signals:
```bash
sudo ./build/analyzer_cli
```

---

## Interactive Controls
- **M**: Main Dashboard - Real-time view of every process and its rank.
- **W**: Waste Analysis - View which processes are being suppressed to save RAM.
- **D**: DS Engine - View internal statistics of the RB-Tree, Skip-List, and Heap.
- **L**: Layer View - See the distribution across L1 Cache, L2 SSD, and L3 Disk.
- **Q**: Quit - Safely exit and restore system defaults.

