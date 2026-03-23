# Adaptive Process Usage Analyzer - Detailed Project Log

This document provides an in-depth explanation of the Adaptive Process Usage Analyzer project. It is structured to answer all aspects of the system, acting as a complete guide for anyone new to the project.

---

## 🔹 1. Project Overview

### What is the main objective of the Adaptive Process Usage Analyzer?
The primary objective is to dynamically monitor active Windows processes, analyze their runtime behaviors, compute a "Hotness Score" based on multiple system metrics, and classify them into priority tiers (HOT, WARM, COLD). This allows the system to generate smart recommendations for memory management.

### Why is process prioritization important in modern operating systems?
* **Resource Optimization:** OSes have finite CPU and RAM. Prioritizing critical tasks prevents system slowdowns.
* **Responsiveness:** Ensures that foreground applications (like a browser or game) get the resources they need instantly.
* **Battery Efficiency:** By accurately identifying "COLD" processes, the system or user can hibernate or kill them, saving battery life on laptops.

### What problem does your system solve compared to existing OS schedulers?
* **Problem Solved:** Traditional OS schedulers rely heavily on basic queues (like Multilevel Feedback Queues) and simple aging. They don't provide a human-readable, multi-metric view combining frequency, recency, and memory-waste detection.
* **Intended Users:** System administrators, power users, and OS developers looking for advanced heuristics to manage RAM and CPU bottlenecks.

---

## 🔹 2. Input and Data Collection

### What types of process data are collected from the operating system?
The system collects the following live metrics:
* **Process Name & PID** (Identifiers)
* **Memory Usage (MB)**
* **CPU Usage (%)**
* **Start Time & Active Time (min)**
* *Simulated Metrics:* Focus Count, Last Used Time, Foreground/Background Duration (added to demonstrate data structure capabilities since raw OS APIs restrict direct access to UI focus events).

### How is live process data retrieved?
We use the **Windows API** via `#include <windows.h>` and `#include <psapi.h>`:
* `CreateToolhelp32Snapshot()` gets the list of running PIDs.
* `OpenProcess()` and `GetProcessMemoryInfo()` fetch memory consumption.
* `GetProcessTimes()` tracks CPU usage and start times.

### Why are these metrics important?
* **CPU & Memory:** Show sheer resource consumption (heaviness).
* **Focus Count & Recency:** Represent the *human element*—a process might use little memory but be highly interacted with (e.g., Notepad).

---

## 🔹 3. System Workflow

*(Note: This section also covers repeated questions regarding modules, inputs/outputs, and step-by-step data movement)*

### Main Components / Modules
1. **Collector Module (`process_collector.h`)**: Interfaces with the OS.
2. **Analyzer Module (`analyzer.h`)**: The brain. Calculates scores and classifies data.
3. **Data Structures Module (`data_structures.h`)**: Stores and organizes data.
4. **Visualizer Module (`visualizer.h`)**: Renders outputs to the user.

### Workflow Step-by-Step (How data moves through the system)
1. **Input / Collection:** The system calls OS APIs to generate a vector of raw process data.
2. **Transformations:** Data is parsed (CPU times converted to percentages, creation times to minutes active).
3. **Distribution to Data Structures:**
   * Dumped into a **Hash Map** for O(1) PID lookups.
   * Inputted into **Fenwick Tree** (to update focus counts) and **Segment Tree** (to record active times).
   * Inputted into the **LRU Doubly Linked List** (to record recency).
4. **Score Calculation:** The Analyzer calculates the *Hotness Score* based on the distributed data.
5. **Ranking & Output:** Computed models are pushed into the **Max Heap**, **Red-Black Tree**, and **Skip List**. The Visualizer then queries these structures based on the user's menu choice.

### How frequently is the data updated?
Data is updated **on-demand** when the user selects Option `[1]` from the menu, representing a snapshot of the live OS state.

---

## 🔹 4. Data Structures Used (Core Part ⭐)

*(Note: This section answers why data structures were chosen and how they improve performance).*

### Hash Map (`unordered_map`)
* **Why it's used:** To act as the central repository for process data.
* **Performance:** Provides **O(1)** average-case time complexity for looking up a process by its PID. Without it, finding a specific process to update its metrics would take O(N) time.

### Heap (Max Heap)
* **Why it's used:** To maintain a priority queue of processes based on their Hotness Score.
* **Performance:** Finding the #1 top priority process takes **O(1)**. Extracting the Top-K processes takes **O(K log N)**, which is significantly faster than sorting the entire list (O(N log N)).

### Red-Black Tree (`std::map`)
* **Why it's used:** To maintain a constantly sorted ranking of processes and to allow efficient Range Queries.
* **Performance:** Allows **O(log N)** time for insertions/deletions and makes queries like "Show me processes with a score between 40 and 60" highly efficient compared to scanning an unsorted array.

### Skip List
* **What it is:** A probabilistic data structure consisting of multi-layered linked lists.
* **Why it's used:** Acts as an alternative to the RB-Tree. It provides **O(log N)** search/insertion but is much easier to implement and maintain under dynamic updates (no complex tree rotations required).

### Doubly Linked List (LRU Mechanism)
* **How it works:** When a process is "focused" or accessed, its node is moved to the **head** (front) of the list.
* **Why it's used:** It perfectly models Recency. The head is always the most recently used, and the tail is the least recently used. Operations (move to front, delete tail) are **O(1)** because we maintain pointers in both directions.

### Segment Tree
* **Why it's used:** For time-based interval analysis.
* **Performance:** Allows querying the sum of active times for any subset range of processes in **O(log N)** time, avoiding O(N) cumulative looping.

### Fenwick Tree (Binary Indexed Tree)
* **Why it's used:** To track cumulative frequencies (e.g., total focus counts across groups of processes).
* **Performance:** Updates and prefix-sum queries both happen in **O(log N)** time using minimal memory compared to a Segment Tree.

---

## 🔹 5. Formula and Score Calculation ⭐

### What is the Hotness Score?
It is a single numerical rating (0–100) that represents how "important" a process is to the user and the system right now.

### The Formula Used
```text
Hotness Score = (0.20 × Frequency) + 
                (0.25 × Recency) + 
                (0.20 × ActiveTime) + 
                (0.15 × Memory) + 
                (0.20 × CPU)
```

### Significance of Variables
* **Frequency (20%):** High frequency means the user opens the app often.
* **Recency (25%):** Weighted the highest. The app you used 2 seconds ago is more important than the app you used 2 hours ago.
* **ActiveTime (20%):** Long-running processes are usually system-critical or primary workspace apps.
* **Memory (15%):** High memory apps are significant, but we weight it slightly lower to avoid over-prioritizing memory leaks or idle bloatware.
* **CPU (20%):** High CPU indicates active computation.

### How does changing weights affect the result?
If we increase the `Memory` weight to 50%, large but idle apps (like a minimized game) would falsely be flagged as "HOT", starving smaller but highly active apps (like an active terminal). Weights are balanced for human-centric priority.

---

## 🔹 6. Classification Logic

### Thresholds
After the Hotness Score is calculated, processes are bucketed:
* **HOT:** Score ≥ 70
* **WARM:** 40 ≤ Score < 70
* **COLD:** Score < 40

### Why is this classification useful?
It translates complex numbers into actionable categories. An OS can apply bulk rules based on categories (e.g., "Suspend all COLD processes when battery hits 20%").

---

## 🔹 7. Output Generation

* **Types of Output:** Tabular CLI tables, ASCII bar charts, categorical lists, and memory-waste alerts.
* **Identifying Top Processes:** Handled seamlessly by popping elements off the **Max Heap**.
* **Detecting Memory Waste:** The system looks for an intersection: It scans for processes where `Memory Usage > System_Average_Memory` AND `Classification == COLD`. This easily flags apps hoarding RAM without being used.

---

## 🔹 8. Visualization

### Graphs Generated (via Python)
1. Usage Time Per Process (Bar)
2. Frequency vs Process (Bar)
3. Memory vs Usage (Scatter Plot)
4. HOT/WARM/COLD Distribution (Pie Chart)
5. Recency Timeline (Bar)
6. Priority Ranking (Bar with threshold lines)

### Why are visualizations important?
Text tables are hard to parse at scale. Scatter plots easily visualize outliers (e.g., high memory, zero usage). Pie charts immediately show the overall health/load status of the system.

---

## 🔹 9. Performance and Optimization

### Time Complexity Table

| Operation/Structure | Time Complexity | Note |
| :- | :- | :- |
| **Hash Map Access** | O(1) average | Rapid PID lookups. |
| **Heap (Extract Max)** | O(log N) | Extremely fast Top-K retrieval. |
| **RB Tree (Insertion)** | O(log N) | Maintains sorting automatically. |
| **Segment Tree (Query)** | O(log N) | Fast range summations. |
| **LRU List (Update/Move)** | O(1) | Real-time recency swapping. |

### Scalability
By utilizing logarithmic `O(log N)` structures, the system can scale to track thousands of concurrent processes without stuttering, far outperforming iterative loops (`O(N)`).

---

## 🔹 10. Real-World Application

### How can this improve OS performance?
An OS utilizing this could implement **Smart Paging**. Instead of paging out RAM based purely on aging, it pages out memory from `COLD` classification processes first, keeping `HOT` data strictly in physical RAM.

### Better than traditional scheduling?
Yes. Traditional schedulers often treat a background service using 5% CPU the exact same as a background UI app using 5% CPU. By factoring in "Focus Count" and "Recency" (UI interactions), this heuristic prioritizes user experience.

---

## 🔹 11. Limitations

* **Assumptions:** The current system assumes higher Active Time linearly correlates with importance, which may not be true for background bloatware.
* **OS Restrictions:** Standard Windows APIs don't easily broadcast "Focus" events for background tracking without deep kernel hooking; thus, we simulate focus count logically for demonstration.

---

## 🔹 12. Future Improvements

* **Machine Learning:** We could implement a clustering algorithm (like K-Means) to dynamically group HOT/WARM/COLD clusters instead of relying on hardcoded (70/40) thresholds.
* **Mobile Extension:** Highly applicable to Android/iOS where aggressive battery management and app resting/suspending is mandatory.

---

## 🔹 13. Viva Quick Questions ⚡

*(Repeated questions marked or grouped)*

* **What is LRU and where is it used?**
  * LRU stands for Least Recently Used. It tracks the chronological usage of items. We use a Doubly Linked List to apply it to Process Recency.
* **Difference between Heap and Tree?**
  * A Heap is a partially ordered tree strictly for finding the Max/Min value quickly. A Tree (like RB-Tree) is fully sorted, allowing range queries and in-order traversal.
* **Why not use only one data structure?**
  * No single data structure does everything perfectly. A Hash Map is O(1) for lookup but cannot sort. A Heap is great for `Max` but terrible for searching by PID. Combining them yields a highly optimized hybrid system.
* **What is the most important part of your system?**
  * The **Hotness Score Algorithm**. It acts as the bridge that converts raw, meaningless system metrics into actionable priority data.
* **What happens if a process suddenly becomes active?**
  * Its `focusCount` increments, its `lastUsedTime` becomes 0 seconds ago (moving to the front of the LRU List), causing its Hotness Score to spike drastically, instantly moving it to `HOT` status in the Heap and Trees.
