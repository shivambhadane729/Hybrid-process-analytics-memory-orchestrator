#ifndef ANALYZER_H
#define ANALYZER_H

#include "data_structures.h"
#include "process_collector.h"
#include "storage_engine.h"
#include "fault_monitor.h"
#include <cmath>
#include <numeric>

using namespace std;

class Analyzer {
private:
    // Weights for hotness score calculation
    static constexpr double W_FREQUENCY  = 0.20;
    static constexpr double W_RECENCY    = 0.25;
    static constexpr double W_ACTIVE     = 0.20;
    static constexpr double W_MEMORY     = 0.15;
    static constexpr double W_CPU        = 0.20;

    // Classification thresholds
    static constexpr double HOT_THRESHOLD  = 70.0;
    static constexpr double WARM_THRESHOLD = 40.0;

public:
    // All data structures
    ProcessHashMap hashMap;
    MaxHeap maxHeap;
    RBTreeRanking rbTree;
    SkipList skipList;
    LRUList lruList;
    SegmentTree segTree;
    FenwickTree fenwickTree;

    // Process index mapping for Fenwick/Segment trees
    unordered_map<int, int> pidToIndex;
    vector<int> indexToPid;

    // === NEW: Storage Engine & Fault Monitor ===
    StorageEngine storageEngine;
    FaultMonitor faultMonitor;

    // Sticky manual relocations: PID -> TargetLayer
    unordered_map<int, string> manualOverrides;

    // =================== CORE FUNCTIONS ===================

    // Collect and store processes
    void collectAndStore() {
        // Clear all data structures
        hashMap.clear();
        maxHeap.clear();
        rbTree.clear();
        skipList.clear();
        lruList.clear();
        segTree.clear();
        fenwickTree.clear();
        pidToIndex.clear();
        indexToPid.clear();

        // Collect live processes
        vector<ProcessData> processes = ProcessCollector::collectLiveProcesses();

        if (processes.empty()) {
            cout << "[WARNING] No processes collected.\n";
            return;
        }

        // Sort processes by focusCount descending to make Fenwick/Segment trees more interesting
        sort(processes.begin(), processes.end(), [](const ProcessData& a, const ProcessData& b) {
            if (a.focusCount != b.focusCount) return a.focusCount > b.focusCount;
            return a.activeTimeMin > b.activeTimeMin;
        });

        // Build index mapping
        int idx = 0;
        for (auto& p : processes) {
            pidToIndex[p.pid] = idx;
            indexToPid.push_back(p.pid);
            idx++;
        }

        // Initialize Fenwick Tree for frequency
        fenwickTree.init((int)processes.size());

        // Initialize Segment Tree for time-based usage
        vector<double> timeSlots(processes.size(), 0);
        for (int i = 0; i < (int)processes.size(); i++) {
            timeSlots[i] = processes[i].activeTimeMin;
        }
        segTree.build(timeSlots);

        // Store in hash map and update Fenwick tree
        for (auto& p : processes) {
            // Update Fenwick tree with focus count
            int fi = pidToIndex[p.pid] + 1; // 1-indexed
            fenwickTree.update(fi, p.focusCount);

            // Calculate hotness score
            calculateScore(p, processes);

            // Store in hash map
            hashMap.insert(p.pid, p);

            // Add to LRU list (sorted by last used time)
            lruList.access(p);
        }

        // Re-get all with scores for heap, RB tree, skip list
        vector<ProcessData> allProcs = hashMap.getAll();

        // Build max heap
        maxHeap.buildFromVector(allProcs);

        // Build RB tree and skip list
        for (auto& p : allProcs) {
            rbTree.insert(p);
            skipList.insert(p);
        }

        // === NEW: Distribute into 3-tier storage ===
        storageEngine.classifyAndDistribute(allProcs);

        // Apply manual overrides if they still exist in the current process list
        for (auto& pair : manualOverrides) {
            int pid = pair.first;
            string target = pair.second;
            string error;
            if (hashMap.find(pid, processes[0])) { // just a check if pid exists
                storageEngine.moveProcess(pid, target, error);
            }
        }
        
        storageEngine.syncLayerToHashMap(hashMap.getMap());

        // Update scores/classifications based on layers (manual moves might have changed them)
        allProcs = hashMap.getAll();
        for (auto& p : allProcs) {
            string layer = storageEngine.findProcessLayer(p.pid);
            if (!layer.empty()) {
                p.storageLayer = layer;
                if (layer == "L1_CACHE") { p.hotnessScore = max(p.hotnessScore, HOT_THRESHOLD); p.classification = "HOT"; }
                else if (layer == "L3_DISK") { p.hotnessScore = min(p.hotnessScore, WARM_THRESHOLD - 5); p.classification = "COLD"; }
                hashMap.insert(p.pid, p);
            }
        }
        
        // Re-sync other structures after all overrides
        syncAllDataStructures();

        cout << "\n  [OK] Collected " << processes.size() << " processes successfully.\n";
        cout << "  [OK] Storage Engine: L1=" << storageEngine.getL1Count()
             << " | L2=" << storageEngine.getL2Count()
             << " | L3=" << storageEngine.getL3Count() << "\n";
    }

    // Calculate hotness score for a process
    void calculateScore(ProcessData& p, const vector<ProcessData>& allProcs) {
        // Normalize each metric to 0-100 range

        // 1. Frequency score (focus count)
        double maxFocus = 1;
        for (auto& proc : allProcs)
            if (proc.focusCount > maxFocus) maxFocus = proc.focusCount;
        double freqScore = (p.focusCount / maxFocus) * 100.0;

        // 2. Recency score (how recently used)
        time_t now = time(nullptr);
        double secSinceUsed = difftime(now, p.lastUsedTime);
        double recencyScore = max(0.0, 100.0 - (secSinceUsed / 36.0)); // decay over 1 hour

        // 3. Active time score
        double maxActive = 1;
        for (auto& proc : allProcs)
            if (proc.activeTimeMin > maxActive) maxActive = proc.activeTimeMin;
        double activeScore = (p.activeTimeMin / maxActive) * 100.0;

        // 4. Memory importance score
        double maxMem = 1;
        for (auto& proc : allProcs)
            if (proc.memoryMB > maxMem) maxMem = proc.memoryMB;
        double memScore = (p.memoryMB / maxMem) * 100.0;

        // 5. CPU activity score
        double cpuScore = min(p.cpuPercent * 5.0, 100.0); // scale up, cap at 100

        // Weighted sum
        p.hotnessScore = W_FREQUENCY * freqScore +
                         W_RECENCY * recencyScore +
                         W_ACTIVE * activeScore +
                         W_MEMORY * memScore +
                         W_CPU * cpuScore;

        // Classify
        if (p.hotnessScore >= HOT_THRESHOLD) {
            p.classification = "HOT";
        } else if (p.hotnessScore >= WARM_THRESHOLD) {
            p.classification = "WARM";
        } else {
            p.classification = "COLD";
        }
    }

    // =================== QUERY FUNCTIONS ===================

    // Get all processes from hash map
    vector<ProcessData> getAllProcesses() const {
        return hashMap.getAll();
    }

    // Get top K from heap
    vector<ProcessData> getTopK(int k) {
        return maxHeap.getTopK(k);
    }

    // Get sorted from RB tree
    vector<ProcessData> getSortedRBTree() const {
        return rbTree.getSorted();
    }

    // Get sorted from skip list
    vector<ProcessData> getSortedSkipList() const {
        return skipList.getSorted();
    }

    // Get recency order from LRU
    vector<ProcessData> getRecencyOrder() const {
        return lruList.getRecencyOrder();
    }

    // Get classified processes
    void getClassified(vector<ProcessData>& hot, vector<ProcessData>& warm, vector<ProcessData>& cold) const {
        vector<ProcessData> all = hashMap.getAll();
        for (auto& p : all) {
            if (p.classification == "HOT") hot.push_back(p);
            else if (p.classification == "WARM") warm.push_back(p);
            else cold.push_back(p);
        }
        // Sort each by score descending
        auto cmp = [](const ProcessData& a, const ProcessData& b) {
            return a.hotnessScore > b.hotnessScore;
        };
        sort(hot.begin(), hot.end(), cmp);
        sort(warm.begin(), warm.end(), cmp);
        sort(cold.begin(), cold.end(), cmp);
    }

    // Memory waste detection: high memory but low usage
    vector<ProcessData> getMemoryWaste() const {
        vector<ProcessData> all = hashMap.getAll();
        vector<ProcessData> waste;

        // Find average memory
        double totalMem = 0;
        for (auto& p : all) totalMem += p.memoryMB;
        double avgMem = totalMem / max((int)all.size(), 1);

        for (auto& p : all) {
            // High memory (above average) AND low score (COLD or low WARM)
            if (p.memoryMB > avgMem && p.hotnessScore < WARM_THRESHOLD + 10) {
                waste.push_back(p);
            }
        }

        // Sort by memory descending
        sort(waste.begin(), waste.end(), [](const ProcessData& a, const ProcessData& b) {
            return a.memoryMB > b.memoryMB;
        });

        return waste;
    }

    // Smart recommendations
    void getRecommendations(vector<ProcessData>& keepHigh, vector<ProcessData>& deprioritize) const {
        vector<ProcessData> all = hashMap.getAll();
        auto cmp = [](const ProcessData& a, const ProcessData& b) {
            return a.hotnessScore > b.hotnessScore;
        };
        sort(all.begin(), all.end(), cmp);

        for (auto& p : all) {
            if (p.classification == "HOT" || (p.classification == "WARM" && p.cpuPercent > 2.0)) {
                keepHigh.push_back(p);
            } else if (p.classification == "COLD" || (p.classification == "WARM" && p.memoryMB > 100 && p.cpuPercent < 1.0)) {
                deprioritize.push_back(p);
            }
        }
    }

    // Segment tree: get total usage for a range of process indices
    double getTimeRangeUsage(int startIdx, int endIdx) const {
        return segTree.query(startIdx, endIdx);
    }

    // Fenwick tree: get cumulative frequency for first k processes
    int getCumulativeFrequency(int k) const {
        return fenwickTree.query(k);
    }

    // RB Tree range query
    vector<ProcessData> getScoreRange(double low, double high) const {
        return rbTree.rangeQuery(low, high);
    }

    bool hasData() const {
        return !hashMap.empty();
    }

    // =================== NEW: STORAGE ENGINE QUERIES ===================

    StorageEngine& getStorageEngine() { return storageEngine; }
    const StorageEngine& getStorageEngine() const { return storageEngine; }

    // Simulate access to a process (promotes through tiers)
    bool simulateAccess(int pid, ProcessData& out) {
        bool found = storageEngine.accessProcess(pid, out);
        if (found) {
            lastActionPid = pid;
            // Live update metrics
            out.focusCount++;
            out.lastUsedTime = time(nullptr);
            
            // Re-calculate score with updated metrics
            vector<ProcessData> all = hashMap.getAll();
            calculateScore(out, all);
            
            // Update in hash map
            hashMap.insert(pid, out);
            
            // Update LRU list
            lruList.access(out);
            
            // Sync updated layer info back to hash map
            storageEngine.syncLayerToHashMap(hashMap.getMap());
            
            // Re-sync all data structures to reflect new scores/layers
            syncAllDataStructures();
        }
        return found;
    }

    // Get fault analysis summaries
    vector<FaultMonitor::FaultSummary> getFaultSummaries() const {
        return FaultMonitor::analyzeByClassification(hashMap.getAll());
    }

    // Get top faulting processes
    vector<ProcessData> getTopFaulters(int n = 10) const {
        return FaultMonitor::getTopFaulters(hashMap.getAll(), n);
    }

    // Get fault-score correlation
    double getFaultCorrelation() const {
        return FaultMonitor::faultScoreCorrelation(hashMap.getAll());
    }

    // Get most swapped processes
    vector<ProcessData> getMostSwapped(int n = 10) const {
        return FaultMonitor::getMostSwapped(hashMap.getAll(), n);
    }

    // =================== RELOCATION FUNCTIONS ===================

    // Rebuild order-based data structures from current hash map
    void syncAllDataStructures() {
        vector<ProcessData> all = hashMap.getAll();
        
        // Rebuild max heap
        maxHeap.buildFromVector(all);
        
        // Rebuild RB tree and skip list
        rbTree.clear();
        skipList.clear();
        for (auto& p : all) {
            rbTree.insert(p);
            skipList.insert(p);
        }
        
        // Update Fenwick Tree with new focus counts using stable indices
        fenwickTree.clear();
        fenwickTree.init((int)all.size());
        
        // Update Segment Tree using stable indices
        vector<double> timeSlots(all.size(), 0.0);
        
        for (auto& p : all) {
            if (pidToIndex.count(p.pid)) {
                int idx = pidToIndex[p.pid];
                fenwickTree.update(idx + 1, p.focusCount);
                if (idx < (int)timeSlots.size()) {
                    timeSlots[idx] = p.activeTimeMin;
                }
            }
        }
        segTree.build(timeSlots);
    }

    int lastActionPid = -1;
    RelocationImpact lastImpact;
    bool hasLastImpact = false;

    // Preview the impact of moving a process to a target layer
    RelocationImpact getRelocationImpact(int pid, const string& targetLayer) const {
        return storageEngine.getRelocationImpact(pid, targetLayer);
    }

    // Move a process to a different storage layer
    bool moveProcess(int pid, const string& targetLayer, string& errorMsg) {
        lastImpact = storageEngine.getRelocationImpact(pid, targetLayer);
        // Pass the hashMap to moveProcess so it can sync evicted processes immediately
        bool ok = storageEngine.moveProcess(pid, targetLayer, errorMsg, &hashMap.getMap());
        
        if (ok) {
            lastActionPid = pid;
            hasLastImpact = true;
            manualOverrides[pid] = targetLayer;
            
            // Update the process in hash map with new metadata
            ProcessData p;
            if (hashMap.find(pid, p)) {
                // Adjust score to match the new layer's expectations
                if (targetLayer == "L1_CACHE") {
                    if (p.hotnessScore < HOT_THRESHOLD) p.hotnessScore = HOT_THRESHOLD + (rand() % 5);
                    p.classification = "HOT";
                } else if (targetLayer == "L2_RAM") {
                    if (p.hotnessScore >= HOT_THRESHOLD) p.hotnessScore = HOT_THRESHOLD - 5;
                    else if (p.hotnessScore < WARM_THRESHOLD) p.hotnessScore = WARM_THRESHOLD + 5;
                    p.classification = "WARM";
                } else if (targetLayer == "L3_DISK") {
                    if (p.hotnessScore >= WARM_THRESHOLD) p.hotnessScore = WARM_THRESHOLD - (5 + (rand() % 10));
                    p.classification = "COLD";
                }
                
                p.storageLayer = targetLayer;
                hashMap.insert(pid, p);
                
                // CRITICAL: Update the process metadata inside StorageEngine too!
                storageEngine.updateProcessMetadata(pid, p.hotnessScore, p.classification);
                
                lruList.access(p);
            }
            
            syncAllDataStructures();
        }
        return ok;
    }

    // Get smart placement suggestions
    vector<PlacementSuggestion> getSmartSuggestions() const {
        return storageEngine.getSmartSuggestions();
    }

    // Find which layer a process is in
    string findProcessLayer(int pid) const {
        return storageEngine.findProcessLayer(pid);
    }
};

#endif // ANALYZER_H
