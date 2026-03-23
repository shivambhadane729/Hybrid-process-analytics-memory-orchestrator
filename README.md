# Adaptive Process Usage Analyzer
## Using Advanced Data Structures for Smart Memory Priority Recommendation

### 🎯 Objective
Monitor live running processes, analyze usage patterns, classify them into **HOT / WARM / COLD**, and generate smart priority recommendations using advanced data structures.

### 🧠 Data Structures Used

| # | Data Structure | Purpose |
|---|---|---|
| 1 | **Hash Map** | Store process data (PID → ProcessData) |
| 2 | **Max Heap** | Rank processes by hotness score, get Top K |
| 3 | **Red-Black Tree** | Sorted ranking, range queries |
| 4 | **Skip List** | Dynamic sorted ranking with fast insertion |
| 5 | **Doubly Linked List (LRU)** | Track recently used processes |
| 6 | **Segment Tree** | Time-based usage analysis (range queries) |
| 7 | **Fenwick Tree (BIT)** | Cumulative frequency tracking |

### 🔄 System Flow
```
Live Processes → Hash Map Storage → Segment Tree (time) + Fenwick Tree (frequency) + LRU List (recency)
→ Score Calculation → Heap Ranking → RB Tree / Skip List Sorting
→ HOT / WARM / COLD Classification → Output + Graphs
```

### 📥 Hotness Score Formula
```
Score = 0.20 × Frequency + 0.25 × Recency + 0.20 × ActiveTime + 0.15 × Memory + 0.20 × CPU
```

Classification:
- **HOT**: Score ≥ 70
- **WARM**: Score 40–70
- **COLD**: Score < 40

### 🏗 Build & Run

#### Compile (C++)
```bash
g++ -std=c++14 -o analyzer.exe main.cpp -lpsapi -static
```

#### Run
```bash
./analyzer.exe
```

#### Generate Graphs (Python)
```bash
pip install pandas matplotlib numpy
python graph_generator.py
```

### 📋 Menu Options
```
[1]  Refresh / Collect Live Process Data
[2]  Display All Processes (Hash Map)
[3]  Display Process Priority Table
[4]  Show HOT / WARM / COLD Classification
[5]  Show Top K Priority Processes (Max Heap)
[6]  Show Memory Waste Detection
[7]  Show Usage Time Report
[8]  Show Frequency Report (Fenwick Tree)
[9]  Show Recency Report (LRU List)
[10] Show Smart Recommendations
[11] Show Time-Based Analysis (Segment Tree)
[12] Show Skip List Ranking
[13] Show Red-Black Tree Sorted View
[14] Score Range Query (RB Tree)
[15] Generate Graph Data (for Python)
[0]  Exit
```

### 📊 Graph Outputs (via Python)
1. Usage Time Per Process (Bar)
2. Frequency vs Process (Bar)
3. Memory vs Usage (Scatter)
4. HOT/WARM/COLD Pie Chart
5. Recency Timeline (Bar)
6. Priority Ranking (Bar)

### 🎤 Viva Explanation
> Our system analyzes live running processes and computes a priority score using advanced data structures. It classifies applications into hot, warm, and cold and provides recommendations to improve system responsiveness.

### 📁 File Structure
```
Code2_DS/
├── main.cpp              # Entry point, menu system
├── data_structures.h     # All 7 data structures
├── process_collector.h   # Windows API process collection
├── analyzer.h            # Score calculation, classification
├── visualizer.h          # CLI output, table formatting
├── graph_generator.py    # Python graph generation
└── README.md             # This file
```

### 🏗 Tech Stack
- **Language**: C++ (compiled with GCC/MinGW)
- **Libraries**: STL, Windows Process API (psapi, tlhelp32)
- **Visualization**: CLI (ANSI colors) + Python (matplotlib)
