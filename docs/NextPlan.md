🎯 Core Concept
Currently your project classifies processes as HOT/WARM/COLD but doesn't do anything with that classification. The upgrade adds actual data movement between 3 storage layers, plus real OS page fault measurement to prove the engine works.

🧩 5 New Features
1. 3-Tier Storage Engine (storage_engine.h — NEW)
Layer	Maps To	DS Used	Rule
L1 Cache	HOT	HashMap (O(1))	Top 10% by score
L2 RAM	WARM	RB-Tree (sorted)	Middle 40%
L3 Disk	COLD	Vector (sequential)	Remaining 50%
Processes promote (Disk→RAM→Cache) when their score rises, and demote when it drops. LRU eviction handles full caches.

2. Real OS Page Fault Monitoring (fault_monitor.h — NEW)
Uses Windows API to collect actual PageFaultCount, PeakWorkingSetSize, and PagefileUsage per process — proving COLD processes truly are swapping to disk.

3. Data Movement Tracking
Logs every promote/demote event with timestamp, reason, and tracks cache hit rate.

4. 4 New Menu Options (16–19)
[16] Storage Layer Distribution (visual capacity bars)
[17] Page Fault Analysis (real OS fault data)
[18] Data Movement Log (promotion/demotion history)
[19] Simulate Access & Optimize (pick a process, watch it promote)
5. 2 New Python Graphs
Storage layer distribution chart
Page faults vs memory scatter plot
📁 Files to Touch
File	Action

data_structures.h
Add new fields to 

ProcessData

process_collector.h
Extract page fault data from OS
storage_engine.h	NEW — 3-tier engine
fault_monitor.h	NEW — OS fault analysis

analyzer.h
Integrate new components

visualizer.h
4 new display functions

main.cpp
4 new menu cases

graph_generator.py
2 new graphs
✅ What Stays the Same
All 7 existing data structures untouched
All 15 existing menu options work exactly as before
Same compile command, same CLI-first approach
Want me to proceed with this plan? I'll implement it in 7 sequential phases, starting from the data layer and building upward.

