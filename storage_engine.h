#ifndef STORAGE_ENGINE_H
#define STORAGE_ENGINE_H

#include "data_structures.h"
#include <unordered_map>
#include <map>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <deque>

using namespace std;

// ========================== PAIRING HEAP ==========================
// Used internally by StorageEngine for efficient decrease-key operations
// when process scores change frequently during promotion/demotion.
// Pairing Heap offers O(1) amortized insert/findMin and O(log n)
// amortized deleteMin/decreaseKey — ideal for a priority-driven
// eviction policy.
// ===================================================================
template <typename T, typename Compare = greater<T>>
class PairingHeap {
private:
    struct PNode {
        T data;
        PNode* child;
        PNode* sibling;
        PNode* prev;   // parent or left-sibling (for decrease-key)

        PNode(const T& d) : data(d), child(nullptr), sibling(nullptr), prev(nullptr) {}
    };

    PNode* root;
    int count;
    Compare cmp;

    // Merge two roots; the "winner" becomes the new root
    PNode* merge(PNode* a, PNode* b) {
        if (!a) return b;
        if (!b) return a;
        // Ensure a is the winner
        if (cmp(b->data, a->data)) swap(a, b);
        // b becomes leftmost child of a
        b->sibling = a->child;
        if (a->child) a->child->prev = b;
        b->prev = a;
        a->child = b;
        a->sibling = nullptr;
        a->prev = nullptr;
        return a;
    }

    // Two-pass merge of sibling list (left-to-right pair, then right-to-left merge)
    PNode* mergePairs(PNode* node) {
        if (!node || !node->sibling) return node;
        PNode* a = node;
        PNode* b = node->sibling;
        PNode* rest = b->sibling;
        a->sibling = nullptr;
        a->prev = nullptr;
        b->sibling = nullptr;
        b->prev = nullptr;
        return merge(merge(a, b), mergePairs(rest));
    }

    void deleteAll(PNode* node) {
        if (!node) return;
        deleteAll(node->child);
        deleteAll(node->sibling);
        delete node;
    }

public:
    PairingHeap() : root(nullptr), count(0) {}
    ~PairingHeap() { deleteAll(root); }

    bool empty() const { return root == nullptr; }
    int size() const { return count; }

    const T& top() const { return root->data; }

    void push(const T& val) {
        PNode* node = new PNode(val);
        root = merge(root, node);
        count++;
    }

    T pop() {
        T val = root->data;
        PNode* children = root->child;
        delete root;
        root = mergePairs(children);
        count--;
        return val;
    }

    void clear() {
        deleteAll(root);
        root = nullptr;
        count = 0;
    }

    // Get all elements (drains then refills — used only for display)
    vector<T> getAll() {
        vector<T> result;
        PairingHeap<T, Compare> temp;
        while (!empty()) {
            T val = pop();
            result.push_back(val);
            temp.push(val);
        }
        // Refill original
        for (auto& v : result) push(v);
        return result;
    }
};

// ========================== DATA MOVEMENT EVENT ==========================
struct MovementEvent {
    int pid;
    string processName;
    string fromLayer;
    string toLayer;
    string reason;
    time_t timestamp;
    double scoreBefore;
    double scoreAfter;
};

// ========================== RELOCATION IMPACT ==========================
struct RelocationImpact {
    int pid;
    string processName;
    string fromLayer;
    string toLayer;
    double hotnessScore;
    // Latency modeling (nanoseconds)
    double latencyBefore;
    double latencyAfter;
    double speedupFactor;     // >1 = faster, <1 = slower
    // Tier balance
    int fromTierCountBefore;
    int fromTierCountAfter;
    int toTierCountBefore;
    int toTierCountAfter;
    int fromTierCapacity;
    int toTierCapacity;
    // Cache efficiency
    double hitRateBefore;
    double hitRateAfter;
    bool isPromotion;         // moving to a faster tier?
    string verdict;           // "Recommended" / "Neutral" / "Not Recommended"
    bool success;
    string errorMsg;
};

// ========================== PLACEMENT SUGGESTION ==========================
struct PlacementSuggestion {
    int pid;
    string processName;
    string currentLayer;
    string suggestedLayer;
    string reason;
    double priority;          // higher = more urgent
    double hotnessScore;
};

// ========================== 3-TIER STORAGE ENGINE ==========================
// L1 Cache (HOT)  — HashMap   O(1)   — Top 10% by score
// L2 RAM   (WARM) — RB-Tree   sorted — Middle 40%
// L3 Disk  (COLD) — Vector    seq    — Remaining 50%
//
// Promotion:  Disk → RAM → Cache  (when score rises)
// Demotion:   Cache → RAM → Disk  (when score drops)
// LRU eviction handles full layers.
// Pairing Heap is used to efficiently track the lowest-score
// process in L1/L2 for eviction (O(1) find-min, O(log n) delete-min).
// ===========================================================================
class StorageEngine {
private:
    // L1 Cache (HOT): HashMap for O(1) access
    unordered_map<int, ProcessData> l1Cache;

    // L2 RAM (WARM): RB-Tree (std::map) sorted by score
    map<double, vector<ProcessData>, greater<double>> l2Ram;

    // L3 Disk (COLD): Vector for sequential access
    vector<ProcessData> l3Disk;

    // Capacity limits (set after first classify call)
    int l1Capacity;
    int l2Capacity;
    int totalProcesses;

    // Movement log
    deque<MovementEvent> movementLog;

    // Cache hit statistics
    int cacheHits;
    int cacheMisses;
    int totalAccesses;

    // Pairing Heap for eviction candidates (min-score at top)
    // We use less<double> so the smallest score is at top for eviction
    struct EvictionEntry {
        double score;
        int pid;
        bool operator>(const EvictionEntry& o) const { return score > o.score; }
        bool operator<(const EvictionEntry& o) const { return score < o.score; }
    };
    struct EvictionCmpMin {
        bool operator()(const EvictionEntry& a, const EvictionEntry& b) const {
            return a.score < b.score; // min at top
        }
    };
    PairingHeap<EvictionEntry, EvictionCmpMin> l1EvictionHeap;

    // Helper: count L2 size
    int l2Size() const {
        int s = 0;
        for (auto& kv : l2Ram) s += (int)kv.second.size();
        return s;
    }

    // Helper: remove from L2 by PID
    bool removeFromL2(int pid) {
        for (auto it = l2Ram.begin(); it != l2Ram.end(); ++it) {
            auto& vec = it->second;
            for (auto vit = vec.begin(); vit != vec.end(); ++vit) {
                if (vit->pid == pid) {
                    vec.erase(vit);
                    if (vec.empty()) l2Ram.erase(it);
                    return true;
                }
            }
        }
        return false;
    }

    // Helper: remove from L3 by PID
    bool removeFromL3(int pid) {
        for (auto it = l3Disk.begin(); it != l3Disk.end(); ++it) {
            if (it->pid == pid) {
                l3Disk.erase(it);
                return true;
            }
        }
        return false;
    }

    // Log a movement event
    void logMovement(const ProcessData& p, const string& from, const string& to,
                     const string& reason, double scoreBefore) {
        MovementEvent evt;
        evt.pid = p.pid;
        evt.processName = p.name;
        evt.fromLayer = from;
        evt.toLayer = to;
        evt.reason = reason;
        evt.timestamp = time(nullptr);
        evt.scoreBefore = scoreBefore;
        evt.scoreAfter = p.hotnessScore;
        movementLog.push_back(evt);
        // Keep last 200 events
        if (movementLog.size() > 200) movementLog.pop_front();
    }

public:
    StorageEngine()
        : l1Capacity(0), l2Capacity(0), totalProcesses(0),
          cacheHits(0), cacheMisses(0), totalAccesses(0) {}

    // =================== CLASSIFY & DISTRIBUTE ===================
    // Called after collectAndStore() to place processes into layers
    void classifyAndDistribute(const vector<ProcessData>& processes, unordered_map<int, string>* history = nullptr) {
        // Clear layers
        l1Cache.clear();
        l2Ram.clear();
        l3Disk.clear();
        l1EvictionHeap.clear();

        totalProcesses = (int)processes.size();
        l1Capacity = max(1, totalProcesses * 10 / 100);   // 10%
        l2Capacity = max(1, totalProcesses * 40 / 100);   // 40%

        // Sort by score descending
        vector<ProcessData> sorted = processes;
        sort(sorted.begin(), sorted.end(),
             [](const ProcessData& a, const ProcessData& b) {
                 return a.hotnessScore > b.hotnessScore;
             });

        for (int i = 0; i < (int)sorted.size(); i++) {
            ProcessData p = sorted[i];
            string targetLayer;
            if (i < l1Capacity) {
                targetLayer = "L1_CACHE";
                l1Cache[p.pid] = p;
                l1EvictionHeap.push({p.hotnessScore, p.pid});
            } else if (i < l1Capacity + l2Capacity) {
                targetLayer = "L2_RAM";
                l2Ram[p.hotnessScore].push_back(p);
            } else {
                targetLayer = "L3_DISK";
                l3Disk.push_back(p);
            }

            p.storageLayer = targetLayer;
            if (history && history->count(p.pid)) {
                string prevLayer = (*history)[p.pid];
                if (prevLayer != targetLayer) {
                    logMovement(p, prevLayer, targetLayer, "Dynamic Hotness Update", p.hotnessScore);
                }
            }
            if (history) (*history)[p.pid] = targetLayer;
        }
    }

    // =================== ACCESS (SIMULATE) ===================
    // Simulate accessing a process — promotes it up if needed
    bool accessProcess(int pid, ProcessData& out) {
        totalAccesses++;

        // Check L1 first (best case)
        auto it = l1Cache.find(pid);
        if (it != l1Cache.end()) {
            cacheHits++;
            out = it->second;
            return true;
        }

        cacheMisses++;

        // Check L2
        for (auto& kv : l2Ram) {
            for (auto& p : kv.second) {
                if (p.pid == pid) {
                    out = p;
                    // Promote to L1 if there's room or evict
                    promoteToL1(p);
                    return true;
                }
            }
        }

        // Check L3
        for (auto& p : l3Disk) {
            if (p.pid == pid) {
                out = p;
                // Promote to L2
                promoteToL2(p);
                return true;
            }
        }

        return false;
    }

    // =================== PROMOTE / DEMOTE ===================
    void promoteToL1(ProcessData p) {
        double oldScore = p.hotnessScore;
        // Boost score slightly on access
        p.hotnessScore = min(100.0, p.hotnessScore + 5.0);

        // If L1 full, evict lowest from L1 to L2
        if ((int)l1Cache.size() >= l1Capacity) {
            evictFromL1();
        }

        removeFromL2(p.pid);
        removeFromL3(p.pid);
        p.storageLayer = "L1_CACHE";
        l1Cache[p.pid] = p;
        l1EvictionHeap.push({p.hotnessScore, p.pid});
        logMovement(p, "L2_RAM", "L1_CACHE", "Access promotion", oldScore);
    }

    void promoteToL2(ProcessData p) {
        double oldScore = p.hotnessScore;
        p.hotnessScore = min(100.0, p.hotnessScore + 3.0);

        removeFromL3(p.pid);
        p.storageLayer = "L2_RAM";
        l2Ram[p.hotnessScore].push_back(p);
        logMovement(p, "L3_DISK", "L2_RAM", "Access promotion", oldScore);
    }

    void evictFromL1(unordered_map<int, ProcessData>* hashMapToSync = nullptr) {
        // Use pairing heap to find lowest-score process in L1
        while (!l1EvictionHeap.empty()) {
            EvictionEntry entry = l1EvictionHeap.pop();
            auto it = l1Cache.find(entry.pid);
            if (it != l1Cache.end()) {
                ProcessData demoted = it->second;
                double oldScore = demoted.hotnessScore;
                l1Cache.erase(it);
                demoted.storageLayer = "L2_RAM";
                l2Ram[demoted.hotnessScore].push_back(demoted);
                
                // If we have a hash map to sync, update it too
                if (hashMapToSync && hashMapToSync->count(demoted.pid)) {
                    (*hashMapToSync)[demoted.pid].storageLayer = "L2_RAM";
                }
                
                logMovement(demoted, "L1_CACHE", "L2_RAM", "LRU eviction (Pairing Heap)", oldScore);
                return;
            }
        }
    }

    // =================== LATENCY CONSTANTS (nanoseconds) ===================
    static constexpr double LATENCY_L1 = 1.0;           // ~1 ns (CPU cache)
    static constexpr double LATENCY_L2 = 100.0;         // ~100 ns (RAM)
    static constexpr double LATENCY_L3 = 10000000.0;    // ~10 ms (Disk)

    static double layerLatency(const string& layer) {
        if (layer == "L1_CACHE") return LATENCY_L1;
        if (layer == "L2_RAM")   return LATENCY_L2;
        return LATENCY_L3;
    }

    static int layerRank(const string& layer) {
        if (layer == "L1_CACHE") return 0;
        if (layer == "L2_RAM")   return 1;
        return 2;
    }

    // =================== GETTERS ===================
    int getL1Count() const { return (int)l1Cache.size(); }
    int getL2Count() const {
        int s = 0;
        for (auto& kv : l2Ram) s += (int)kv.second.size();
        return s;
    }
    int getL3Count() const { return (int)l3Disk.size(); }
    int getL1Capacity() const { return l1Capacity; }
    int getL2Capacity() const { return l2Capacity; }
    int getTotalProcesses() const { return totalProcesses; }

    double getCacheHitRate() const {
        return totalAccesses > 0 ? (double)cacheHits / totalAccesses * 100.0 : 0.0;
    }
    int getCacheHits() const { return cacheHits; }
    int getCacheMisses() const { return cacheMisses; }
    int getTotalAccesses() const { return totalAccesses; }

    vector<ProcessData> getL1Processes() const {
        vector<ProcessData> result;
        for (auto& kv : l1Cache) result.push_back(kv.second);
        sort(result.begin(), result.end(), [](const ProcessData& a, const ProcessData& b) {
            return a.hotnessScore > b.hotnessScore;
        });
        return result;
    }

    vector<ProcessData> getL2Processes() const {
        vector<ProcessData> result;
        for (auto& kv : l2Ram)
            for (auto& p : kv.second)
                result.push_back(p);
        return result;
    }

    vector<ProcessData> getL3Processes() const {
        return l3Disk;
    }

    const deque<MovementEvent>& getMovementLog() const {
        return movementLog;
    }

    // =================== FIND PROCESS LAYER ===================
    string findProcessLayer(int pid) const {
        if (l1Cache.count(pid)) return "L1_CACHE";
        for (auto& kv : l2Ram)
            for (auto& p : kv.second)
                if (p.pid == pid) return "L2_RAM";
        for (auto& p : l3Disk)
            if (p.pid == pid) return "L3_DISK";
        return "";
    }

    // Find and return the process data from any layer
    bool findProcess(int pid, ProcessData& out) const {
        auto it = l1Cache.find(pid);
        if (it != l1Cache.end()) { out = it->second; return true; }
        for (auto& kv : l2Ram)
            for (auto& p : kv.second)
                if (p.pid == pid) { out = p; return true; }
        for (auto& p : l3Disk)
            if (p.pid == pid) { out = p; return true; }
        return false;
    }

    int getTierCount(const string& layer) const {
        if (layer == "L1_CACHE") return getL1Count();
        if (layer == "L2_RAM")   return getL2Count();
        return getL3Count();
    }

    int getTierCapacity(const string& layer) const {
        if (layer == "L1_CACHE") return l1Capacity;
        if (layer == "L2_RAM")   return l2Capacity;
        return totalProcesses - l1Capacity - l2Capacity;
    }

    // =================== RELOCATION IMPACT PREVIEW ===================
    RelocationImpact getRelocationImpact(int pid, const string& targetLayer) const {
        RelocationImpact impact;
        impact.pid = pid;
        impact.success = false;

        ProcessData proc;
        if (!findProcess(pid, proc)) {
            impact.errorMsg = "Process not found";
            return impact;
        }

        string currentLayer = findProcessLayer(pid);
        if (currentLayer == targetLayer) {
            impact.errorMsg = "Already in target layer";
            return impact;
        }

        impact.processName = proc.name;
        impact.fromLayer = currentLayer;
        impact.toLayer = targetLayer;
        impact.hotnessScore = proc.hotnessScore;

        // Latency comparison
        impact.latencyBefore = layerLatency(currentLayer);
        impact.latencyAfter  = layerLatency(targetLayer);
        impact.speedupFactor = impact.latencyBefore / impact.latencyAfter;
        impact.isPromotion = (layerRank(targetLayer) < layerRank(currentLayer));

        // Tier balance
        impact.fromTierCountBefore = getTierCount(currentLayer);
        impact.fromTierCountAfter  = impact.fromTierCountBefore - 1;
        impact.toTierCountBefore   = getTierCount(targetLayer);
        impact.toTierCountAfter    = impact.toTierCountBefore + 1;
        impact.fromTierCapacity    = getTierCapacity(currentLayer);
        impact.toTierCapacity      = getTierCapacity(targetLayer);

        // Cache hit rate estimate
        impact.hitRateBefore = getCacheHitRate();
        if (targetLayer == "L1_CACHE")
            impact.hitRateAfter = totalAccesses > 0
                ? (double)(cacheHits + 1) / (totalAccesses + 1) * 100.0
                : impact.hitRateBefore;
        else if (currentLayer == "L1_CACHE")
            impact.hitRateAfter = totalAccesses > 0
                ? (double)max(0, cacheHits - 1) / max(1, totalAccesses) * 100.0
                : impact.hitRateBefore;
        else
            impact.hitRateAfter = impact.hitRateBefore;

        // Verdict
        bool scoreMatchesTier = false;
        if (targetLayer == "L1_CACHE" && proc.hotnessScore >= 70.0) scoreMatchesTier = true;
        if (targetLayer == "L2_RAM" && proc.hotnessScore >= 40.0 && proc.hotnessScore < 70.0) scoreMatchesTier = true;
        if (targetLayer == "L3_DISK" && proc.hotnessScore < 40.0) scoreMatchesTier = true;

        bool overCapacity = (impact.toTierCountAfter > impact.toTierCapacity);

        if (scoreMatchesTier && !overCapacity)
            impact.verdict = "Recommended";
        else if (overCapacity)
            impact.verdict = "Not Recommended";
        else if (impact.isPromotion && proc.hotnessScore >= 40.0)
            impact.verdict = "Neutral";
        else if (!impact.isPromotion && proc.hotnessScore < 40.0)
            impact.verdict = "Neutral";
        else
            impact.verdict = "Not Recommended";

        impact.success = true;
        return impact;
    }

    // Moves a process from its current layer to targetLayer.
    // Handles eviction automatically if the target is full.
    bool moveProcess(int pid, const string& targetLayer, string& errorMsg, unordered_map<int, ProcessData>* hashMapToSync = nullptr) {
        ProcessData proc;
        if (!findProcess(pid, proc)) {
            errorMsg = "Process not found";
            return false;
        }

        string fromLayer = findProcessLayer(pid);
        if (fromLayer == targetLayer) {
            errorMsg = "Already in target layer";
            return false;
        }

        double oldScore = proc.hotnessScore;

        // Remove from source layer
        if (fromLayer == "L1_CACHE") {
            l1Cache.erase(pid);
        } else if (fromLayer == "L2_RAM") {
            removeFromL2(pid);
        } else {
            removeFromL3(pid);
        }

        // Insert into target layer, evict if necessary
        proc.storageLayer = targetLayer;

        if (targetLayer == "L1_CACHE") {
            if ((int)l1Cache.size() >= l1Capacity)
                evictFromL1(hashMapToSync);
            l1Cache[pid] = proc;
            l1EvictionHeap.push({proc.hotnessScore, pid});
        } else if (targetLayer == "L2_RAM") {
            l2Ram[proc.hotnessScore].push_back(proc);
        } else {
            l3Disk.push_back(proc);
        }

        string reason = "Manual relocation (" + fromLayer + " -> " + targetLayer + ")";
        logMovement(proc, fromLayer, targetLayer, reason, oldScore);

        return true;
    }

    // Update metadata for a process in whatever layer it is in
    void updateProcessMetadata(int pid, double score, const string& classification) {
        if (l1Cache.count(pid)) {
            l1Cache[pid].hotnessScore = score;
            l1Cache[pid].classification = classification;
            l1EvictionHeap.push({score, pid}); // Re-push with new score
        } else {
            // Find in L2
            for (auto& kv : l2Ram) {
                auto& vec = kv.second;
                for (auto it = vec.begin(); it != vec.end(); ++it) {
                    if (it->pid == pid) {
                        ProcessData p = *it;
                        vec.erase(it);
                        if (vec.empty()) l2Ram.erase(kv.first);
                        p.hotnessScore = score;
                        p.classification = classification;
                        l2Ram[score].push_back(p);
                        return;
                    }
                }
            }
            // Find in L3
            for (auto& p : l3Disk) {
                if (p.pid == pid) {
                    p.hotnessScore = score;
                    p.classification = classification;
                    return;
                }
            }
        }
    }

    // =================== SMART SUGGESTIONS ===================
    vector<PlacementSuggestion> getSmartSuggestions() const {
        vector<PlacementSuggestion> suggestions;

        // Check L1 (Cache) for low-score processes that should be demoted
        for (auto& kv : l1Cache) {
            const ProcessData& p = kv.second;
            if (p.hotnessScore < 40.0) {
                suggestions.push_back({
                    p.pid, p.name, "L1_CACHE", "L3_DISK",
                    "Cold process occupying cache (Score: " + to_string((int)p.hotnessScore) + ")",
                    90.0 - p.hotnessScore, p.hotnessScore
                });
            } else if (p.hotnessScore < 70.0) {
                suggestions.push_back({
                    p.pid, p.name, "L1_CACHE", "L2_RAM",
                    "Warm process in cache, better suited for RAM (Score: " + to_string((int)p.hotnessScore) + ")",
                    50.0 - (p.hotnessScore - 40.0), p.hotnessScore
                });
            }
        }

        // Check L2 (RAM) for misplaced processes
        for (auto& kv : l2Ram) {
            for (auto& p : kv.second) {
                if (p.hotnessScore >= 70.0) {
                    suggestions.push_back({
                        p.pid, p.name, "L2_RAM", "L1_CACHE",
                        "Hot process stuck in RAM, promote to cache (Score: " + to_string((int)p.hotnessScore) + ")",
                        p.hotnessScore - 60.0, p.hotnessScore
                    });
                } else if (p.hotnessScore < 30.0) {
                    suggestions.push_back({
                        p.pid, p.name, "L2_RAM", "L3_DISK",
                        "Cold process in RAM, demote to disk (Score: " + to_string((int)p.hotnessScore) + ")",
                        40.0 - p.hotnessScore, p.hotnessScore
                    });
                }
            }
        }

        // Check L3 (Disk) for hot processes that should be promoted
        for (auto& p : l3Disk) {
            if (p.hotnessScore >= 70.0) {
                suggestions.push_back({
                    p.pid, p.name, "L3_DISK", "L1_CACHE",
                    "Hot process on disk — severe latency! Promote to cache (Score: " + to_string((int)p.hotnessScore) + ")",
                    p.hotnessScore, p.hotnessScore
                });
            } else if (p.hotnessScore >= 40.0) {
                suggestions.push_back({
                    p.pid, p.name, "L3_DISK", "L2_RAM",
                    "Warm process on disk, promote to RAM (Score: " + to_string((int)p.hotnessScore) + ")",
                    p.hotnessScore - 30.0, p.hotnessScore
                });
            }
        }

        // Sort by priority descending
        sort(suggestions.begin(), suggestions.end(),
             [](const PlacementSuggestion& a, const PlacementSuggestion& b) {
                 return a.priority > b.priority;
             });

        return suggestions;
    }

    // =================== SYNC ===================
    void syncLayerToHashMap(unordered_map<int, ProcessData>& hashMapTable) {
        for (auto& kv : l1Cache)
            if (hashMapTable.count(kv.first))
                hashMapTable[kv.first].storageLayer = "L1_CACHE";

        for (auto& kv : l2Ram)
            for (auto& p : kv.second)
                if (hashMapTable.count(p.pid))
                    hashMapTable[p.pid].storageLayer = "L2_RAM";

        for (auto& p : l3Disk)
            if (hashMapTable.count(p.pid))
                hashMapTable[p.pid].storageLayer = "L3_DISK";
    }
};

#endif // STORAGE_ENGINE_H
