#ifndef ANALYZER_H
#define ANALYZER_H

#include "data_structures.h"
#include "process_collector.h"
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

        cout << "\n  [OK] Collected " << processes.size() << " processes successfully.\n";
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
};

#endif // ANALYZER_H
