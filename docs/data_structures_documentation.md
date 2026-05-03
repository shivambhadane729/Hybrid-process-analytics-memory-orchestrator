# 📊 Data Structure Analysis & Visualization Documentation

This document provides a comprehensive technical overview of the data structures implemented in the **Hybrid Process Analytics Memory Orchestrator**. It details the rationale behind choosing each structure, their computational complexities, and the logic used to plot them in the graphical interface.

---

## 1. Data Structure Matrix

| Data Structure | Primary Role | Selection Criteria | Time Complexity | Plotting Style |
| :--- | :--- | :--- | :--- | :--- |
| **Hash Map** | PID Mapping | O(1) random access for unique identifiers. | O(1) Avg | Table / Internal |
| **Max Heap** | Priority Ranking | Efficiently identifies "hottest" processes without full sorting. | O(1) Peek, O(log N) Extract | Binary Tree |
| **Red-Black Tree** | Range Queries | Maintains balanced sorted order for score-range filtering. | O(log N) All | Balanced Tree |
| **Skip List** | Fast Search | Probabilistic alternative to trees; easier to parallelize and visualize. | O(log N) Avg | Multi-level Grid |
| **LRU List** | Recency Tracking | Tracks process usage order to decide memory eviction. | O(1) Access | Doubly Linked List |
| **Segment Tree** | Aggregate Analysis | Performs range sum queries on process usage data. | O(log N) Query | Range-based Tree |
| **Fenwick Tree** | Cumulative Freq | Tracks running totals (e.g., total focus counts). | O(log N) Query | Prefix Sum Bar Chart |
| **Pairing Heap** | Eviction Engine | High-performance priority queue for the Storage Engine. | O(1) Insert/Find | Internal |

---

## 2. Selection Criteria (Rationale)

### 2.1 Why Max Heap?
In an OS environment, we frequently need the **Top K** most resource-intensive processes. While sorting takes $O(N \log N)$, a Max Heap allows us to extract the top elements in $O(K \log N)$. This is critical for real-time dashboards where we only care about the top offenders.

### 2.2 Why Red-Black Tree vs. Skip List?
We implemented both to handle **Sorted Ranking**:
- **Red-Black Tree**: Used for stable, guaranteed $O(\log N)$ performance. It is ideal for the **L2 RAM** layer where we need to maintain a perfectly balanced set of "Warm" processes.
- **Skip List**: Provides a more "visual" representation of levels. It allows for faster range iteration compared to standard trees and is often used in modern databases (like Redis) for sorted sets.

### 2.3 Why Segment Tree & Fenwick Tree?
These are specialized for **Analytical Queries**:
- **Segment Tree**: If a user wants to know the "Total memory used by processes with PIDs between 1000 and 5000", the Segment Tree answers this in $O(\log N)$ instead of $O(N)$.
- **Fenwick Tree (BIT)**: Optimized for cumulative statistics. It helps in calculating the "Cumulative Focus Count" or "Memory Usage Percentiles" efficiently.

### 2.4 Why LRU List?
Memory management follows the **Principle of Locality**. The LRU (Least Recently Used) list is the gold standard for cache eviction. By combining a **Doubly Linked List** with a **Hash Map**, we achieve $O(1)$ updates every time a process is "touched" or accessed.

---

## 3. Visualization Logic (How They are Plot)

The visualization is built using the **Qt Graphics View Framework**, which allows for high-performance rendering of thousands of interactive items.

### 3.1 Common Drawing Components
- **Nodes**: Represented as `QGraphicsEllipseItem` or `QGraphicsRectItem`.
- **Edges**: Represented as `QGraphicsLineItem` with linear gradients to show data flow direction.
- **Gradients**: Nodes use **Radial Gradients** to create a 3D "premium" feel. Colors are dynamically mapped to the process's classification:
  - <span style="color:#ef4444">■</span> **HOT**: Red/High Intensity
  - <span style="color:#f59e0b">■</span> **WARM**: Orange/Amber
  - <span style="color:#10b981">■</span> **COLD**: Green/Teal

### 3.2 Structure-Specific Plotting
#### **Max Heap (Binary Tree Plot)**
- **Logic**: Calculated using the standard array-to-tree mapping formula: `Parent(i) = (i-1)/2`.
- **Layout**: Nodes are placed in levels. Each level $L$ has $2^L$ nodes, and the horizontal spacing is halved at every level to prevent overlap.

#### **Red-Black Tree (Balanced Plot)**
- **Logic**: Uses a recursive layout algorithm.
- **Visuals**: Nodes are colored strictly based on their "Black" or "Red" status (modeled here by depth parity) to demonstrate the balancing property.

#### **Skip List (Multi-Level Plot)**
- **Logic**: Displays the "layers" vertically.
- **Visuals**: Shows how a search can "skip" over nodes by following the higher-level horizontal arrows, eventually dropping down to the base level (L0).

#### **LRU List (Chain Plot)**
- **Logic**: A simple horizontal linear layout.
- **Visuals**: Features **HEAD** and **TAIL** indicators. **Green arrows** point to the `next` node (more recent), while **Red arrows** point to the `prev` node (less recent).

#### **Segment Tree (Range Plot)**
- **Logic**: Each node is labeled with the range it covers (e.g., `[0-7]`) and the sum of its children.
- **Visuals**: Highlights the specific leaf node corresponding to the **Last Action PID** to show where an update occurred.

---

## 4. Architectural Integration

The `DSVisualizerTab` acts as a listener to the `Analyzer` class.

1. **The Pulse**: Every time the `Analyzer` refreshes (collects live data), it signals the UI.
2. **The Sweep**: The UI clears the `QGraphicsScene` for each data structure.
3. **The Reconstruction**: The UI iterates through the data structures and reconstructs the visual scene from scratch.
4. **Interaction**: Hovering over a node provides detailed tooltips (from the `ProcessData` struct), and the **Impact Panel** shows real-time stats (latency, speedup) when processes move between layers.

---
