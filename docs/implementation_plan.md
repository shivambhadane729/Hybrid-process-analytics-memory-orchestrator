# C++ Qt Widgets GUI for Hybrid Process Analytics Memory Orchestrator

## Background

The project has a working C++ backend. The GUI must be **C++ only using Qt Widgets**. No Qt SDK is currently installed, so we install it first via `aqtinstall`.

## Setup: Install Qt 5.15.2 + MinGW 8.1

The user's system has:
- GCC 6.3.0 (old MinGW) — too old for Qt
- `winget` available
- Anaconda Python (can run `pip install aqtinstall`)

**Installation steps** (automated):
1. `pip install aqtinstall`
2. `aqt install-qt --outputdir C:\Qt windows desktop 5.15.2 win64_mingw81` — installs Qt libs + headers
3. `aqt install-tool --outputdir C:\Qt windows desktop tools_mingw qt.tools.win64_mingw810` — installs matching MinGW 8.1
4. Add to PATH: `C:\Qt\5.15.2\mingw81_64\bin` and `C:\Qt\Tools\mingw810_64\bin`
5. Verify: `qmake --version` and `g++ --version`

## Architecture

```
┌────────────────────────────────────────────────────────┐
│                  Qt GUI Application                     │
│                                                         │
│  main_gui.cpp (replaces main.cpp)                       │
│  └─ QApplication → MainWindow                          │
│                                                         │
│  MainWindow (QMainWindow + QTabWidget)                  │
│  ├─ DashboardTab     (process table + side panels)      │
│  ├─ StorageTab       (3-tier viz + movement log)        │
│  ├─ DSVisualizerTab  (heap/tree/LRU/etc graphics)       │
│  └─ FaultTab         (fault tables + correlation)       │
│                                                         │
│  Shared Analyzer* ←── single instance, passed by ref    │
│  QTimer (2s auto-refresh) → Analyzer::collectAndStore() │
└────────────────────────────────────────────────────────┘
         │
         │ Direct C++ method calls (no subprocess, no CSV)
         ▼
┌────────────────────────────────────────────────────────┐
│          Existing C++ Backend (UNCHANGED)               │
│  Analyzer → ProcessCollector → Data Structures          │
│           → StorageEngine → FaultMonitor                │
└────────────────────────────────────────────────────────┘
```

> [!IMPORTANT]
> The GUI calls Analyzer methods **directly** in C++ — no subprocess, no CSV intermediary. This is pure native C++ with Qt Widgets.

---

## Proposed Changes

### Phase 0: Install Qt SDK

Install Qt 5.15.2 + MinGW 8.1 via aqtinstall. Verify `qmake` and `g++` work.

---

### Phase 1: GUI Entry Point

#### [NEW] gui/main_gui.cpp
- Initialize `QApplication`
- Create `MainWindow`
- Instantiate single shared `Analyzer` object
- Pass `Analyzer*` to `MainWindow`
- `app.exec()`

#### [NEW] gui/mainwindow.h / gui/mainwindow.cpp
- `MainWindow : QMainWindow`
- Central widget: `QTabWidget` with 4 tabs
- Owns `Analyzer*`
- "Refresh" button in toolbar → `Analyzer::collectAndStore()`
- `QTimer` (2 sec) for auto-refresh
- Dark theme stylesheet

#### [NEW] gui/gui.pro
- Qt project file for `qmake`
- Links against: `Qt5Widgets`, `Qt5Core`, `Qt5Gui`
- Adds `-lpsapi` for Windows process APIs
- Includes all existing headers (`data_structures.h`, `analyzer.h`, etc.)

---

### Phase 2: Dashboard Tab

#### [NEW] gui/dashboard_tab.h / gui/dashboard_tab.cpp
- `DashboardTab : QWidget`
- Main `QTableWidget` populated from `Analyzer::getAllProcesses()`
  - Columns: PID, Name, CPU%, Memory(MB), PageFaults, Score, Classification
  - Row colors: HOT=#e94560, WARM=#f5a623, COLD=#0f3460
- Side panels (QDockWidget or splitter):
  - **Top K** panel: `QListWidget` from `Analyzer::getTopK(5)`
  - **Memory Waste** panel: `QListWidget` from `Analyzer::getMemoryWaste()`
  - **Recommendations** panel: Keep/Deprioritize split from `Analyzer::getRecommendations()`
- Refresh button + auto-refresh indicator (QLabel with last-refresh timestamp)

---

### Phase 3: Storage Engine Tab

#### [NEW] gui/storage_tab.h / gui/storage_tab.cpp
- `StorageTab : QWidget`
- 3-tier visualization:
  - L1 Cache / L2 RAM / L3 Disk as colored capacity bars (custom painted QProgressBar)
  - Process list per tier: `StorageEngine::getL1Processes()`, etc.
- Cache stats: hit rate, hits, misses, total accesses
- Movement log: `QTableWidget` from `StorageEngine::getMovementLog()`
- Simulate Access panel: `QSpinBox` (PID) + QPushButton → `Analyzer::simulateAccess()`

---

### Phase 4: Data Structure Visualization Tab

#### [NEW] gui/ds_viz_tab.h / gui/ds_viz_tab.cpp
- `DSVisualizerTab : QWidget` with sub-tab selector (QComboBox or QTabWidget)
- Sub-views:
  - **Max Heap**: Tree drawn via QGraphicsView (circles + lines), top-K highlighted red
  - **Red-Black Tree**: Sorted tree nodes with color coding
  - **Skip List**: Multi-level horizontal linked structure
  - **LRU List**: Horizontal chain (most recent → least recent)
  - **Segment Tree**: Binary tree showing range segments
  - **Fenwick Tree**: Bar visualization of prefix sums
- All use **real data** from `Analyzer`'s actual data structure instances
- Animate highlight on refresh

---

### Phase 5: Fault Monitor Tab

#### [NEW] gui/fault_tab.h / gui/fault_tab.cpp
- `FaultTab : QWidget`
- Fault summary table: `FaultMonitor::analyzeByClassification()` → Class, Count, Avg Faults, Avg Pagefile(KB), Avg PeakWS(KB)
- Top faulters table: `FaultMonitor::getTopFaulters(10)`
- Correlation display: `FaultMonitor::faultScoreCorrelation()` with interpretation label

---

## Verification Plan

### Automated Tests
- `qmake gui/gui.pro && mingw32-make` — compiles without errors
- Run exe → window appears with all 4 tabs
- Click "Refresh" → table populates with real running processes
- Data structures tab shows correct real data

### Manual Verification
- HOT/WARM/COLD colors match expected thresholds
- Storage tier counts sum to total process count
- Heap visualization shows highest score at root
- LRU shows most recent process first
