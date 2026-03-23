# Code Execution Flow & Architecture Breakdown (`codeExplanation.md`)

This document traces the exact path the code takes when the user selects a menu option in `main.cpp`. It shows how data flows between files: `main.cpp` ➔ `analyzer.h` ➔ `process_collector.h` ➔ `data_structures.h` ➔ `visualizer.h`.

---

## 🔹 Option 1: Refresh / Collect Live Process Data
**Goal:** Pull live processes from Windows, calculate scores, and populate all data structures.

1. **`main.cpp` (Line ~95):** User presses `1`. Calls `analyzer.collectAndStore()`.
2. **`analyzer.h` (`collectAndStore`):** Clears all existing data structures in memory.
3. **`process_collector.h` (`collectLiveProcesses`):** 
   * **(Windows API Execution):** Uses `#include <windows.h>` and `#include <psapi.h>`.
   * `CreateToolhelp32Snapshot()` generates the snapshot list of running PIDs.
   * `Process32First()` / `Process32Next()` loops through them.
   * `OpenProcess()` links to the PID, and `GetProcessMemoryInfo()` fetches physical `WorkingSetSize` (Memory consumption).
   * `GetProcessTimes()` fetches `ftCreation` (Start time) and `kernel/user` execution times to calculate the CPU percentage.
   * Duplicate names (like 10 instances of Chrome) are merged into one `ProcessData` object.
   * Returns a `vector<ProcessData>`.
4. **`analyzer.h` (Back to `collectAndStore`):**
   * Pre-loads the **Segment Tree** with active times and the **Fenwick Tree** with focus counts.
   * Calls `calculateScore()` which runs the mathematical algorithm for the Hotness Score.
   * Stores the processes into the **Hash Map** (`PID` ➔ `ProcessData`).
   * Sends the processes into the **LRU Doubly Linked List** (recency cache).
   * Bulks builds the **Max Heap**, pushes into the **Red-Black Tree** and the **Skip List**.

---

## 🔹 Option 2: Display All Processes
**Goal:** Show a raw list of everything we collected.

1. **`main.cpp`:** User presses `2`. Calls `analyzer.getAllProcesses()`.
2. **`analyzer.h`:** Directly accesses `hashMap.getAll()` from `data_structures.h` (Iterates through the unordered map).
3. **`visualizer.h` (`displayAllProcesses`):** Loops through the vector and prints clean columns (Name, PID, Mem, CPU, Active Min).

---

## 🔹 Option 3: Display Process Priority Table
**Goal:** Show all processes sorted by their Hotness priority.

1. **`main.cpp`:** Calls `analyzer.getAllProcesses()`.
2. **`visualizer.h` (`displayPriorityTable`):**
   * Grabs the raw Hash Map vector.
   * Applies `std::sort` in descending order based on `p.hotnessScore`.
   * Prints the table, using ANSI color codes to highlight `p.classification`.

---

## 🔹 Option 4: Show HOT / WARM / COLD Classification
**Goal:** Bucket the processes into 3 distinct priority tiers.

1. **`main.cpp`:** Calls `analyzer.getClassified(hot, warm, cold)`.
2. **`analyzer.h`:** Iterates over the raw Hash Map vector and pushes processes into three reference vectors (`hot`, `warm`, `cold`) depending on their pre-calculated internal string property. Sorts them individually.
3. **`visualizer.h` (`displayClassification`):** Iterates over the 3 vectors and prints ANSI-colored buckets.

---

## 🔹 Option 5: Show Top K Priority Processes
**Goal:** Swiftly extract ONLY the highest priority processes dynamically.

1. **`main.cpp`:** Asks user for `k` (e.g., 5). Calls `analyzer.getTopK(k)`.
2. **`analyzer.h`:** Calls `maxHeap.getTopK(k)`.
3. **`data_structures.h` (Max Heap):**
   * Copies the heap locally so it doesn't destroy the root structure.
   * Pops the root (`heap[0]`), replacing with the back, then triggers `heapifyDown()` doing this `k` times.
4. **`visualizer.h` (`displayTopK`):** Formats and prints the extracted Top K array.

---

## 🔹 Option 6: Show Memory Waste Detection
**Goal:** Identify heavy background apps that are doing relatively nothing.

1. **`main.cpp`:** Calls `analyzer.getMemoryWaste()`.
2. **`analyzer.h` (`getMemoryWaste`):**
   * Averages out the system memory mathematically.
   * Loops through the Hash Map: IF `memoryMB > average` AND Score is low (`< WARM_THRESHOLD + 10`), it is flagged as Waste.
3. **`visualizer.h` (`displayMemoryWaste`):** Iterates over flagged apps, summing total potentially recoverable MB of RAM.

---

## 🔹 Option 7: Usage Time Report
**Goal:** Show total execution lifespan in an active Bar Chart.

1. **`main.cpp`:** Fetches Hash Map dump from analyzer.
2. **`visualizer.h` (`displayUsageTimeReport`):** Sorts by `activeTimeMin` descending. Calculates the screen-width scaling logic and paints an ASCII Bar array using `#` characters based on maximum time.

---

## 🔹 Option 8: Frequency Report
**Goal:** Output individual focus counts and the aggregate system total.

1. **`main.cpp`:** 
   * Fetches Hash Map vector for the visualizer to render standard Frequency bar charts.
   * Calls `analyzer.getCumulativeFrequency(n)`.
2. **`analyzer.h`:** Forwards call to `fenwickTree.query(n)`.
3. **`data_structures.h` (Fenwick Tree):** In `O(log N)` time, traverses the `bit[]` array summing up the prefix nodes to yield standard/cumulative Focus Counts accurately.

---

## 🔹 Option 9: Recency Report
**Goal:** Present processes ordered chronologically by interaction.

1. **`main.cpp`:** Calls `analyzer.getRecencyOrder()`.
2. **`analyzer.h`:** Accesses `lruList.getRecencyOrder()`.
3. **`data_structures.h` (LRU List):** Takes the `head` pointer of the Doubly Linked List (Most Recently Used node) and walks completely to the `tail` node. Returns the traced chronological path as a vector.
4. **`visualizer.h` (`displayRecencyReport`):** Translates seconds back into readable time (`2 min ago`, `1 hr ago`).

---

## 🔹 Option 10: Smart Recommendations
**Goal:** Deliver direct, human-readable instructions on what to close vs keep.

1. **`main.cpp`:** Calls `analyzer.getRecommendations()`.
2. **`analyzer.h`:** Identifies `keepHigh` processes (HOT processes + WARM ones with > 2% CPU load). Identifies `deprioritize` processes (COLD + WARM processes with massive memory > 100MB but 0% CPU use).
3. **`visualizer.h`:** Prints output clearly, displaying calculated potential RAM savings if closed.

---

## 🔹 Option 11: Time-Based Analysis (Segment Tree)
**Goal:** Divide the usage time landscape dynamically into Quartiles using range arrays.

1. **`main.cpp`:** Calls `visualizer.displayTimeAnalysis(all, segTree, pidToIndex)`.
2. **`visualizer.h` & `data_structures.h` (Segment Tree):**
   * Takes the total process count (`n`). Determines Quartiles `q1, q2, q3`.
   * Queries the tree bounds `segTree.query(l, r)`.
   * **Segment Tree** traverses nodes covering matching ranges and merges left/right children sum returning `O(log N)` total time without iteration. Prints percentile consumption breakdowns.

---

## 🔹 Option 12: Skip List Ranking
**Goal:** Render the dynamic Skip List configuration paths.

1. **`main.cpp`:** Calls `analyzer.getSortedSkipList()`.
2. **`data_structures.h` (Skip List):** Follows the base layer `forward[0]` pointers of the probabilistically generated node layers all the way down, yielding a completely sorted sequence without ever touching a sorting algorithm. 

---

## 🔹 Option 13: Red-Black Tree Sorted View
**Goal:** Show processes mapped via a self-balancing binary search tree.

1. **`main.cpp`:** Calls `analyzer.getSortedRBTree()`.
2. **`data_structures.h` (RB Tree):** Sweeps `std::map` iterators (`.begin()` to `.end()`). Because `std::map<double, ... greater<double>>` automatically balanced nodes on insert (handling collisions natively as `vector<ProcessData>`), walking the tree produces descending sorted iterations perfectly.

---

## 🔹 Option 14: Score Range Query
**Goal:** Execute a bounded search inquiry through the Red-Black tree structure.

1. **`main.cpp`:** User scopes `low` and `high` scores via `cin`. Calls `analyzer.getScoreRange(low, high)`.
2. **`data_structures.h` (RB Tree):** Iterates the map. Because of tree mapping logic, immediately strips away anything outside the explicit `low`-`high` bounds.
3. **`visualizer.h`:** Renders only nodes matching that score bounded array.

---

## 🔹 Option 15: Generate Graph Data (CSV Export)
**Goal:** Transform inner C++ states into Python-readable file structures.

1. **`main.cpp`:** Calls `visualizer::generateGraphData(hashMapData)`.
2. **`visualizer.h`:** Opens a standard `std::ofstream file("process_data.csv")`. Walks the process dataset and maps exact integer float states mapped against comma delimiters.
3. **Result:** Creates `.csv` on disk, allowing parallel offline access to Python backend scripts `graph_generator.py`.
