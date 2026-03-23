#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cmath>
#include <functional>

using namespace std;

// ========================== PROCESS DATA ==========================
struct ProcessData {
    string name;
    int pid;
    double memoryMB;       // Memory usage in MB
    double cpuPercent;     // CPU usage %
    time_t startTime;
    double activeTimeMin;  // Active time in minutes
    time_t lastUsedTime;
    int focusCount;        // How many times focused/opened
    double foregroundDur;  // Foreground duration in minutes
    double backgroundDur;  // Background duration in minutes
    double hotnessScore;   // Computed score
    string classification; // HOT / WARM / COLD

    ProcessData()
        : pid(0), memoryMB(0), cpuPercent(0), startTime(0),
          activeTimeMin(0), lastUsedTime(0), focusCount(0),
          foregroundDur(0), backgroundDur(0), hotnessScore(0),
          classification("COLD") {}
};

// ========================== 1. HASH MAP ==========================
// Using std::unordered_map<int, ProcessData> for PID -> ProcessData
// Wrapper class for clarity
class ProcessHashMap {
private:
    unordered_map<int, ProcessData> table;

public:
    void insert(int pid, const ProcessData& data) {
        table[pid] = data;
    }

    bool find(int pid, ProcessData& out) const {
        auto it = table.find(pid);
        if (it != table.end()) {
            out = it->second;
            return true;
        }
        return false;
    }

    void remove(int pid) {
        table.erase(pid);
    }

    void clear() {
        table.clear();
    }

    vector<ProcessData> getAll() const {
        vector<ProcessData> result;
        result.reserve(table.size());
        for (auto& p : table) {
            result.push_back(p.second);
        }
        return result;
    }

    size_t size() const { return table.size(); }

    bool empty() const { return table.empty(); }

    unordered_map<int, ProcessData>& getMap() { return table; }
    const unordered_map<int, ProcessData>& getMap() const { return table; }
};

// ========================== 2. MAX HEAP ==========================
class MaxHeap {
private:
    vector<ProcessData> heap;

    void heapifyUp(int idx) {
        while (idx > 0) {
            int parent = (idx - 1) / 2;
            if (heap[idx].hotnessScore > heap[parent].hotnessScore) {
                swap(heap[idx], heap[parent]);
                idx = parent;
            } else break;
        }
    }

    void heapifyDown(int idx) {
        int n = (int)heap.size();
        while (true) {
            int largest = idx;
            int left = 2 * idx + 1;
            int right = 2 * idx + 2;
            if (left < n && heap[left].hotnessScore > heap[largest].hotnessScore)
                largest = left;
            if (right < n && heap[right].hotnessScore > heap[largest].hotnessScore)
                largest = right;
            if (largest != idx) {
                swap(heap[idx], heap[largest]);
                idx = largest;
            } else break;
        }
    }

public:
    void clear() { heap.clear(); }

    void insert(const ProcessData& data) {
        heap.push_back(data);
        heapifyUp((int)heap.size() - 1);
    }

    ProcessData extractMax() {
        ProcessData top = heap[0];
        heap[0] = heap.back();
        heap.pop_back();
        if (!heap.empty()) heapifyDown(0);
        return top;
    }

    ProcessData peekMax() const {
        return heap[0];
    }

    bool empty() const { return heap.empty(); }
    int size() const { return (int)heap.size(); }

    // Get top K processes
    vector<ProcessData> getTopK(int k) {
        vector<ProcessData> result;
        // Make a copy to avoid destroying the heap
        MaxHeap copy = *this;
        for (int i = 0; i < k && !copy.empty(); i++) {
            result.push_back(copy.extractMax());
        }
        return result;
    }

    void buildFromVector(const vector<ProcessData>& data) {
        heap = data;
        // Build heap bottom-up
        for (int i = (int)heap.size() / 2 - 1; i >= 0; i--) {
            heapifyDown(i);
        }
    }
};

// ========================== 3. RED-BLACK TREE ==========================
// Using std::map as Red-Black Tree (it is internally implemented as one)
// Key: hotnessScore, Value: vector of ProcessData (multiple processes can have same score)
class RBTreeRanking {
private:
    // Using map<double, vector<ProcessData>> sorted by score descending
    map<double, vector<ProcessData>, greater<double>> tree;

public:
    void clear() { tree.clear(); }

    void insert(const ProcessData& data) {
        tree[data.hotnessScore].push_back(data);
    }

    // Get all processes sorted by score (descending)
    vector<ProcessData> getSorted() const {
        vector<ProcessData> result;
        for (auto it = tree.begin(); it != tree.end(); ++it) {
            for (size_t i = 0; i < it->second.size(); i++)
                result.push_back(it->second[i]);
        }
        return result;
    }

    // Range query: processes with score in [low, high]
    vector<ProcessData> rangeQuery(double low, double high) const {
        vector<ProcessData> result;
        // Since we use greater<double>, iterate and check
        for (auto it = tree.begin(); it != tree.end(); ++it) {
            if (it->first >= low && it->first <= high) {
                for (size_t i = 0; i < it->second.size(); i++)
                    result.push_back(it->second[i]);
            }
        }
        return result;
    }

    // Top K processes
    vector<ProcessData> topK(int k) const {
        vector<ProcessData> result;
        int count = 0;
        for (auto it = tree.begin(); it != tree.end(); ++it) {
            for (size_t i = 0; i < it->second.size(); i++) {
                if (count >= k) return result;
                result.push_back(it->second[i]);
                count++;
            }
        }
        return result;
    }

    size_t size() const {
        size_t s = 0;
        for (auto it = tree.begin(); it != tree.end(); ++it)
            s += it->second.size();
        return s;
    }

    bool empty() const { return tree.empty(); }
};

// ========================== 4. SKIP LIST ==========================
class SkipList {
private:
    static const int MAX_LEVEL = 16;

    struct SkipNode {
        ProcessData data;
        vector<SkipNode*> forward;

        SkipNode(int level, const ProcessData& d = ProcessData())
            : data(d), forward(level + 1, nullptr) {}
    };

    SkipNode* header;
    int currentLevel;
    int nodeCount;

    int randomLevel() {
        int lvl = 0;
        while (lvl < MAX_LEVEL && (rand() % 2) == 0)
            lvl++;
        return lvl;
    }

public:
    SkipList() : currentLevel(0), nodeCount(0) {
        header = new SkipNode(MAX_LEVEL);
    }

    ~SkipList() {
        SkipNode* curr = header->forward[0];
        while (curr) {
            SkipNode* next = curr->forward[0];
            delete curr;
            curr = next;
        }
        delete header;
    }

    void clear() {
        SkipNode* curr = header->forward[0];
        while (curr) {
            SkipNode* next = curr->forward[0];
            delete curr;
            curr = next;
        }
        for (int i = 0; i <= MAX_LEVEL; i++)
            header->forward[i] = nullptr;
        currentLevel = 0;
        nodeCount = 0;
    }

    // Insert sorted by hotnessScore descending
    void insert(const ProcessData& data) {
        vector<SkipNode*> update(MAX_LEVEL + 1);
        SkipNode* curr = header;

        for (int i = currentLevel; i >= 0; i--) {
            while (curr->forward[i] != nullptr &&
                   curr->forward[i]->data.hotnessScore > data.hotnessScore) {
                curr = curr->forward[i];
            }
            update[i] = curr;
        }

        int lvl = randomLevel();
        if (lvl > currentLevel) {
            for (int i = currentLevel + 1; i <= lvl; i++)
                update[i] = header;
            currentLevel = lvl;
        }

        SkipNode* newNode = new SkipNode(lvl, data);
        for (int i = 0; i <= lvl; i++) {
            newNode->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = newNode;
        }
        nodeCount++;
    }

    // Get all sorted
    vector<ProcessData> getSorted() const {
        vector<ProcessData> result;
        SkipNode* curr = header->forward[0];
        while (curr) {
            result.push_back(curr->data);
            curr = curr->forward[0];
        }
        return result;
    }

    // Get top K
    vector<ProcessData> topK(int k) const {
        vector<ProcessData> result;
        SkipNode* curr = header->forward[0];
        int count = 0;
        while (curr && count < k) {
            result.push_back(curr->data);
            curr = curr->forward[0];
            count++;
        }
        return result;
    }

    int size() const { return nodeCount; }
    bool empty() const { return nodeCount == 0; }
};

// ========================== 5. LRU LIST (Doubly Linked List) ==========================
class LRUList {
private:
    struct LRUNode {
        ProcessData data;
        LRUNode* prev;
        LRUNode* next;
        LRUNode(const ProcessData& d) : data(d), prev(nullptr), next(nullptr) {}
    };

    LRUNode* head;
    LRUNode* tail;
    unordered_map<int, LRUNode*> nodeMap; // PID -> node
    int capacity;
    int currentSize;

public:
    LRUList(int cap = 1000) : head(nullptr), tail(nullptr), capacity(cap), currentSize(0) {}

    ~LRUList() {
        LRUNode* curr = head;
        while (curr) {
            LRUNode* next = curr->next;
            delete curr;
            curr = next;
        }
    }

    void clear() {
        LRUNode* curr = head;
        while (curr) {
            LRUNode* next = curr->next;
            delete curr;
            curr = next;
        }
        head = tail = nullptr;
        nodeMap.clear();
        currentSize = 0;
    }

    // Move to front (most recently used)
    void access(const ProcessData& data) {
        if (nodeMap.count(data.pid)) {
            // Move existing node to front
            LRUNode* node = nodeMap[data.pid];
            node->data = data; // Update data
            moveToFront(node);
        } else {
            // Insert new node at front
            LRUNode* node = new LRUNode(data);
            nodeMap[data.pid] = node;
            addToFront(node);
            currentSize++;

            // Evict if over capacity
            if (currentSize > capacity) {
                removeTail();
                currentSize--;
            }
        }
    }

    // Get recency order (most recent first)
    vector<ProcessData> getRecencyOrder() const {
        vector<ProcessData> result;
        LRUNode* curr = head;
        while (curr) {
            result.push_back(curr->data);
            curr = curr->next;
        }
        return result;
    }

    int size() const { return currentSize; }
    bool empty() const { return currentSize == 0; }

private:
    void addToFront(LRUNode* node) {
        node->next = head;
        node->prev = nullptr;
        if (head) head->prev = node;
        head = node;
        if (!tail) tail = node;
    }

    void removeNode(LRUNode* node) {
        if (node->prev) node->prev->next = node->next;
        else head = node->next;

        if (node->next) node->next->prev = node->prev;
        else tail = node->prev;
    }

    void moveToFront(LRUNode* node) {
        if (node == head) return;
        removeNode(node);
        addToFront(node);
    }

    void removeTail() {
        if (!tail) return;
        nodeMap.erase(tail->data.pid);
        LRUNode* prev = tail->prev;
        delete tail;
        tail = prev;
        if (tail) tail->next = nullptr;
        else head = nullptr;
    }
};

// ========================== 6. SEGMENT TREE ==========================
// For time-based usage analysis
// Stores usage values in time slots and supports range queries
class SegmentTree {
private:
    vector<double> tree;
    int n;

    void buildHelper(const vector<double>& arr, int node, int start, int end) {
        if (start == end) {
            tree[node] = arr[start];
        } else {
            int mid = (start + end) / 2;
            buildHelper(arr, 2 * node, start, mid);
            buildHelper(arr, 2 * node + 1, mid + 1, end);
            tree[node] = tree[2 * node] + tree[2 * node + 1];
        }
    }

    void updateHelper(int node, int start, int end, int idx, double val) {
        if (start == end) {
            tree[node] = val;
        } else {
            int mid = (start + end) / 2;
            if (idx <= mid)
                updateHelper(2 * node, start, mid, idx, val);
            else
                updateHelper(2 * node + 1, mid + 1, end, idx, val);
            tree[node] = tree[2 * node] + tree[2 * node + 1];
        }
    }

    double queryHelper(int node, int start, int end, int l, int r) const {
        if (r < start || end < l) return 0;
        if (l <= start && end <= r) return tree[node];
        int mid = (start + end) / 2;
        return queryHelper(2 * node, start, mid, l, r) +
               queryHelper(2 * node + 1, mid + 1, end, l, r);
    }

public:
    SegmentTree() : n(0) {}

    void build(const vector<double>& arr) {
        n = (int)arr.size();
        if (n == 0) return;
        tree.assign(4 * n, 0);
        buildHelper(arr, 1, 0, n - 1);
    }

    void build(int size) {
        n = size;
        if (n == 0) return;
        tree.assign(4 * n, 0);
    }

    void update(int idx, double val) {
        if (idx < 0 || idx >= n) return;
        updateHelper(1, 0, n - 1, idx, val);
    }

    // Query sum in range [l, r]
    double query(int l, int r) const {
        if (n == 0 || l > r || l < 0 || r >= n) return 0;
        return queryHelper(1, 0, n - 1, l, r);
    }

    int getSize() const { return n; }

    void clear() {
        tree.clear();
        n = 0;
    }
};

// ========================== 7. FENWICK TREE (BIT) ==========================
// For cumulative frequency tracking
class FenwickTree {
private:
    vector<int> bit;
    int n;

public:
    FenwickTree() : n(0) {}

    void init(int size) {
        n = size;
        bit.assign(n + 1, 0);
    }

    // Add val to index idx (1-indexed)
    void update(int idx, int val) {
        for (; idx <= n; idx += idx & (-idx))
            bit[idx] += val;
    }

    // Get prefix sum [1, idx]
    int query(int idx) const {
        int sum = 0;
        for (int i = idx; i > 0; i -= i & (-i))
            sum += bit[i];
        return sum;
    }

    // Range sum [l, r] (1-indexed)
    int rangeQuery(int l, int r) const {
        if (l > r) return 0;
        return query(r) - query(l - 1);
    }

    int getSize() const { return n; }

    void clear() {
        bit.clear();
        n = 0;
    }
};

#endif // DATA_STRUCTURES_H
