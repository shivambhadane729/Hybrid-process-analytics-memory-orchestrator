# Qt GUI Implementation Tasks

## Phase 0: Install Qt SDK
- [/] Install aqtinstall via pip
- [ ] Install Qt 5.15.2 win64_mingw81
- [ ] Install MinGW 8.1 toolchain
- [ ] Verify qmake + g++ work

## Phase 1: GUI Entry Point
- [ ] Create gui/ directory structure
- [ ] Create gui.pro (qmake project file)
- [ ] Create main_gui.cpp
- [ ] Create mainwindow.h / mainwindow.cpp
- [ ] Compile and verify window opens

## Phase 2: Dashboard Tab
- [ ] Create dashboard_tab.h / dashboard_tab.cpp
- [ ] Process table with color coding
- [ ] Side panels (Top K, Memory Waste, Recommendations)
- [ ] Refresh button + auto-refresh timer

## Phase 3: Storage Engine Tab
- [ ] Create storage_tab.h / storage_tab.cpp
- [ ] 3-tier capacity bars + process lists
- [ ] Movement log table
- [ ] Simulate Access (PID input + button)

## Phase 4: Data Structure Visualization
- [ ] Create ds_viz_tab.h / ds_viz_tab.cpp
- [ ] Max Heap tree visualization
- [ ] Red-Black Tree visualization
- [ ] Skip List visualization
- [ ] LRU List visualization
- [ ] Segment Tree visualization
- [ ] Fenwick Tree visualization

## Phase 5: Fault Monitor Tab
- [ ] Create fault_tab.h / fault_tab.cpp
- [ ] Fault summary table
- [ ] Top faulters table
- [ ] Correlation display
