# Detailed Code Explanation (`DetailedExplanationCode.md`)

This document provides a highly granular explanation of every class, structure, and function within the project's source code. It is designed for developers or students who want to understand the "under-the-hood" implementation details.

---

## 📂 1. `data_structures.h`
This header file defines the core data structures used for storing, sorting, and querying process information.

### 🏗️ `ProcessData` (Struct)
*   **Purpose:** The fundamental data unit representing a single system process.
*   **Fields:**
    *   `name`: The executable name (e.g., "chrome.exe").
    *   `pid`: Process ID.
    *   `memoryMB`: RAM consumption in Megabytes.
    *   `cpuPercent`: Percentage of CPU time used.
    *   `startTime`: Timestamp of when the process started.
    *   `activeTimeMin`: Total lifespan of the process in minutes.
    *   `lastUsedTime`: Simulated timestamp of last user interaction.
    *   `focusCount`: Number of times the app was interacted with.
    *   `foregroundDur` / `backgroundDur`: Time spent in front vs. behind other apps.
    *   `hotnessScore`: The final 0–100 calculated priority value.
    *   `classification`: String label ("HOT", "WARM", or "COLD").

### 🏗️ `ProcessHashMap` (Class Wrapper)
*   **Purpose:** Provides O(1) average-case lookup using `std::unordered_map`.
*   **Functions:**
    *   `insert(pid, data)`: Maps a PID to its data object.
    *   `find(pid, out)`: Retrieves data by PID. Returns `true` if found.
    *   `getAll()`: Flattens the map into a vector for sorting and visualization.

### 🏗️ `MaxHeap` (Class)
*   **Purpose:** Maintains a binary heap to extract high-priority processes efficiently.
*   **Key Logic:** `heapifyUp` and `heapifyDown` maintain the property where the parent node always has a higher `hotnessScore` than its children.
*   **Functions:**
    *   `extractMax()`: Removes and returns the highest-scoring process. Complexity: **O(log N)**.
    *   `getTopK(k)`: Returns a vector of the top `k` processes by repeatedly extracting the max.
    *   `buildFromVector()`: Converts an unsorted vector into a heap using the bottom-up "Heapify" method in **O(N)**.

### 🏗️ `RBTreeRanking` (Class Wrapper)
*   **Purpose:** Uses a Red-Black Tree (via `std::map`) to store processes in a permanently sorted state.
*   **Key Logic:** Uses `std::map<double, vector<ProcessData>, greater<double>>` to handle multiple processes with the same score and keep them sorted descending.
*   **Functions:**
    *   `rangeQuery(low, high)`: Returns all processes whose scores fall within the specified bounds. Complexity: **O(log N + M)** where M is the result count.
    *   `getSorted()`: Returns all processes in descending score order.

### 🏗️ `SkipList` (Class)
*   **Purpose:** A probabilistic, linked-list-based alternative to balanced trees.
*   **Key Logic:** Uses multiple layers of pointers. Each node has a random "level." Higher layers "skip" over nodes to allow **O(log N)** search/insertion.
*   **Functions:**
    *   `insert(data)`: Places process data into the list at a calculated random level.
    *   `getSorted()`: Traverses the base layer (level 0) to get all processes in order.

### 🏗️ `LRUList` (Class)
*   **Purpose:** A Doubly Linked List combined with a Hash Map to track user recency.
*   **Functions:**
    *   `access(data)`: If a PID exists, it's moved to the `head` (Most Recently Used). If not, it's added to the `head`.
    *   `removeTail()`: When the list hits capacity, the last node (Least Recently Used) is deleted. Complexity: **O(1)**.
    *   `getRecencyOrder()`: Walks from head to tail to show which apps were used most recently.

### 🏗️ `SegmentTree` (Class)
*   **Purpose:** Provides range sum queries for time-based analysis.
*   **Functions:**
    *   `build(vector)`: Constructs the tree where each leafy node is a process's `activeTimeMin`.
    *   `query(l, r)`: Returns the sum of active times for all processes between index `l` and `r`. Complexity: **O(log N)**.

### 🏗️ `FenwickTree` (Class)
*   **Purpose:** Efficiently calculates prefix sums (cumulative frequencies).
*   **Functions:**
    *   `update(idx, val)`: Adds to the frequency at a specific index. Uses bitwise `idx += idx & (-idx)`.
    *   `query(idx)`: Returns the total sum of focus counts from the start of the list to the index. Complexity: **O(log N)**.

---

## 📂 2. `process_collector.h`
This file contains the logic for interacting with the Windows Operating System.

*   **`collectLiveProcesses()`**: 
    *   Uses `CreateToolhelp32Snapshot` to get a list of all active PIDs.
    *   Calls `OpenProcess` for each PID.
    *   Uses `GetProcessMemoryInfo` for RAM usage.
    *   Uses `GetProcessTimes` to calculate `activeTimeMin` and `cpuPercent`.
    *   **Logic:** Aggregates data by process name (e.g., all "chrome.exe" instances are combined into one entry).
*   **`cleanName()`**: Removes ".exe" and capitalizes the name for a professional display.

---

## 📂 3. `analyzer.h`
The "Brain" of the project that links the data structures and mathematical logic.

*   **`collectAndStore()`**: The master function. It clears the old data, calls the `ProcessCollector`, calculates scores for everyone, and populates all 7 data structures simultaneously.
*   **`calculateScore()`**: 
    *   **Normalization:** Scales every metric (Focus, Recency, Memory, etc.) to a 0–100 scale relative to the maximum value found in the current system state.
    *   **Weighted Sum:** Applies the weights (e.g., 25% for Recency) to generate the final `hotnessScore`.
    *   **Classification:**Assigns "HOT", "WARM", or "COLD" based on thresholds (70/40).
*   **`getMemoryWaste()`**: Logic that flags processes which have high RAM usage but a low hotness score (meaning they are heavy but idle).
*   **`getRecommendations()`**: Logic that decides which apps should stay in high priority and which can be safely closed or deprioritized.

---

## 📂 4. `visualizer.h`
The presentation layer that handles all terminal output.

*   **Styling Functions:** `printHeader`, `printSeparator`, and `classColor` (which uses ANSI escape codes like `\033[1;31m` to turn the text Red).
*   **Table Displays:** Functions like `displayPriorityTable` use `std::setw` to ensure clean columns.
*   **Bar Charts:** `displayUsageTimeReport` and `displayFrequencyReport` calculate the percentage of a line to fill with `#` characters to create a visual ASCII graph.
*   **Graph Data Export:** `generateGraphData()` writes the entire internal process state into a `process_data.csv` file for external analysis by Python scripts.

---

## 📂 5. `main.cpp`
The entry point and the user interaction loop.

*   **`enableANSIColors()`**: A critical Windows-specific function that enables Virtual Terminal Processing so the terminal can show colors instead of weird symbols.
*   **`main()` Loop:**
    *   Initializes the `Analyzer` object.
    *   Runs a `while(true)` loop that prints the menu using `Visualizer`.
    *   Uses a `switch-case` statement to handle the 15 user options.
    *   **Safety:** The `checkData()` function ensures the user doesn't try to analyze empty data before running Option 1.
