/*
 * ============================================================
 *   ADAPTIVE PROCESS USAGE ANALYZER
 *   Using Advanced Data Structures for Smart Memory
 *   Priority Recommendation
 * ============================================================
 *
 * Data Structures Used:
 *   1. Hash Map       - Store process data (PID -> ProcessData)
 *   2. Max Heap       - Ranking processes by hotness score
 *   3. Red-Black Tree - Sorted order, range queries, Top K
 *   4. Skip List      - Dynamic sorted ranking
 *   5. Doubly Linked List (LRU) - Recency tracking
 *   6. Segment Tree   - Time-based usage analysis
 *   7. Fenwick Tree   - Cumulative frequency tracking
 *
 * System Flow:
 *   Live Processes -> Hash Map -> Score Calculation ->
 *   Heap Ranking -> RB Tree/Skip List Sorting ->
 *   HOT/WARM/COLD Classification -> Output
 *
 * ============================================================
 */

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>

// Windows-specific includes
#ifdef _WIN32
    #include <windows.h>
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
#endif

#include "data_structures.h"
#include "process_collector.h"
#include "analyzer.h"
#include "visualizer.h"

using namespace std;

// Enable ANSI colors on Windows
void enableANSIColors() {
    #ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    #endif
}

void printWelcome() {
    cout << "\n";
    cout << "  \033[1;36m" << string(60, '*') << "\033[0m\n";
    cout << "  \033[1;36m*\033[0m                                                          \033[1;36m*\033[0m\n";
    cout << "  \033[1;36m*\033[0m   \033[1;33mADAPTIVE PROCESS USAGE ANALYZER\033[0m                     \033[1;36m*\033[0m\n";
    cout << "  \033[1;36m*\033[0m   \033[1;37mAdvanced Data Structures for Smart Memory\033[0m           \033[1;36m*\033[0m\n";
    cout << "  \033[1;36m*\033[0m   \033[1;37mPriority Recommendation System\033[0m                      \033[1;36m*\033[0m\n";
    cout << "  \033[1;36m*\033[0m                                                          \033[1;36m*\033[0m\n";
    cout << "  \033[1;36m*\033[0m   DS Used: HashMap | MaxHeap | RBTree | SkipList         \033[1;36m*\033[0m\n";
    cout << "  \033[1;36m*\033[0m            LRU List | SegmentTree | FenwickTree          \033[1;36m*\033[0m\n";
    cout << "  \033[1;36m*\033[0m                                                          \033[1;36m*\033[0m\n";
    cout << "  \033[1;36m" << string(60, '*') << "\033[0m\n";
    cout << "\n";
}

bool checkData(const Analyzer& analyzer) {
    if (!analyzer.hasData()) {
        cout << "\n  \033[1;31m[!] No data loaded. Please select option [1] first to collect process data.\033[0m\n";
        return false;
    }
    return true;
}

void waitForEnter() {
    cout << "\n  Press Enter to continue...";
    cin.ignore();
    cin.get();
}

int main() {
    // Setup
    enableANSIColors();
    srand((unsigned)time(nullptr));

    Analyzer analyzer;
    int choice;

    printWelcome();

    while (true) {
        Visualizer::printMenu();
        cin >> choice;

        switch (choice) {

        case 1: {
            // =================== REFRESH / COLLECT DATA ===================
            Visualizer::printHeader("COLLECTING LIVE PROCESS DATA");
            cout << "\n  Scanning running processes...\n";
            analyzer.collectAndStore();
            cout << "\n  Data structures populated:\n";
            cout << "    - Hash Map:      " << analyzer.hashMap.size() << " entries\n";
            cout << "    - Max Heap:      Ready\n";
            cout << "    - Red-Black Tree: Ready\n";
            cout << "    - Skip List:     Ready\n";
            cout << "    - LRU List:      Ready\n";
            cout << "    - Segment Tree:  " << analyzer.segTree.getSize() << " slots\n";
            cout << "    - Fenwick Tree:  " << analyzer.fenwickTree.getSize() << " entries\n";
            break;
        }

        case 2: {
            // =================== DISPLAY ALL PROCESSES ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> all = analyzer.getAllProcesses();
            Visualizer::displayAllProcesses(all);
            break;
        }

        case 3: {
            // =================== PRIORITY TABLE ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> all = analyzer.getAllProcesses();
            Visualizer::displayPriorityTable(all);
            break;
        }

        case 4: {
            // =================== HOT/WARM/COLD ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> hot, warm, cold;
            analyzer.getClassified(hot, warm, cold);
            Visualizer::displayClassification(hot, warm, cold);
            break;
        }

        case 5: {
            // =================== TOP K (MAX HEAP) ===================
            if (!checkData(analyzer)) break;
            int k;
            cout << "  Enter K (number of top processes): ";
            cin >> k;
            if (k <= 0) {
                cout << "  Invalid K value.\n";
                break;
            }
            vector<ProcessData> topK = analyzer.getTopK(k);
            Visualizer::displayTopK(topK, k);
            break;
        }

        case 6: {
            // =================== MEMORY WASTE ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> waste = analyzer.getMemoryWaste();
            Visualizer::displayMemoryWaste(waste);
            break;
        }

        case 7: {
            // =================== USAGE TIME REPORT ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> all = analyzer.getAllProcesses();
            Visualizer::displayUsageTimeReport(all);
            break;
        }

        case 8: {
            // =================== FREQUENCY REPORT (FENWICK TREE) ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> all = analyzer.getAllProcesses();
            Visualizer::displayFrequencyReport(all);

            // Show Fenwick Tree cumulative frequency
            Visualizer::printSubHeader("Fenwick Tree Cumulative Frequency");
            int n = analyzer.fenwickTree.getSize();
            cout << "  Total cumulative focus count (all): " << analyzer.getCumulativeFrequency(n) << "\n";
            if (n >= 5) {
                cout << "  Cumulative focus count (first 5): " << analyzer.getCumulativeFrequency(5) << "\n";
            }
            if (n >= 10) {
                cout << "  Cumulative focus count (first 10): " << analyzer.getCumulativeFrequency(10) << "\n";
            }
            break;
        }

        case 9: {
            // =================== RECENCY REPORT (LRU) ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> recency = analyzer.getRecencyOrder();
            Visualizer::displayRecencyReport(recency);
            break;
        }

        case 10: {
            // =================== SMART RECOMMENDATIONS ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> keepHigh, deprioritize;
            analyzer.getRecommendations(keepHigh, deprioritize);
            Visualizer::displayRecommendations(keepHigh, deprioritize);
            break;
        }

        case 11: {
            // =================== TIME ANALYSIS (SEGMENT TREE) ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> all = analyzer.getAllProcesses();
            Visualizer::displayTimeAnalysis(all, analyzer.segTree, analyzer.pidToIndex);
            break;
        }

        case 12: {
            // =================== SKIP LIST RANKING ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> sorted = analyzer.getSortedSkipList();
            Visualizer::displaySkipListRanking(sorted);
            break;
        }

        case 13: {
            // =================== RB TREE SORTED VIEW ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> sorted = analyzer.getSortedRBTree();
            Visualizer::displayRBTreeView(sorted);
            break;
        }

        case 14: {
            // =================== SCORE RANGE QUERY ===================
            if (!checkData(analyzer)) break;
            double low, high;
            cout << "  Enter score range [low high]: ";
            cin >> low >> high;
            vector<ProcessData> results = analyzer.getScoreRange(low, high);
            Visualizer::displayScoreRange(results, low, high);
            break;
        }

        case 15: {
            // =================== GENERATE GRAPH DATA ===================
            if (!checkData(analyzer)) break;
            vector<ProcessData> all = analyzer.getAllProcesses();
            Visualizer::generateGraphData(all, "process_data.csv");
            break;
        }

        case 16: {
            // =================== STORAGE LAYER DISTRIBUTION ===================
            if (!checkData(analyzer)) break;
            Visualizer::displayStorageDistribution(analyzer.getStorageEngine());
            break;
        }

        case 17: {
            // =================== PAGE FAULT ANALYSIS ===================
            if (!checkData(analyzer)) break;
            auto summaries = analyzer.getFaultSummaries();
            auto topFaulters = analyzer.getTopFaulters(10);
            double corr = analyzer.getFaultCorrelation();
            Visualizer::displayPageFaultAnalysis(summaries, topFaulters, corr);
            break;
        }

        case 18: {
            // =================== DATA MOVEMENT LOG ===================
            if (!checkData(analyzer)) break;
            Visualizer::displayMovementLog(analyzer.getStorageEngine().getMovementLog());
            break;
        }

        case 19: {
            // =================== SIMULATE ACCESS & OPTIMIZE ===================
            if (!checkData(analyzer)) break;
            int pid;
            cout << "  Enter PID to simulate access: ";
            cin >> pid;
            ProcessData result;
            bool found = analyzer.simulateAccess(pid, result);
            Visualizer::displayAccessResult(result, found);
            if (found) {
                cout << "\n  Updated Storage: L1=" << analyzer.getStorageEngine().getL1Count()
                     << " | L2=" << analyzer.getStorageEngine().getL2Count()
                     << " | L3=" << analyzer.getStorageEngine().getL3Count() << "\n";
            }
            break;
        }

        case 0: {
            // =================== EXIT ===================
            cout << "\n  \033[1;33mThank you for using Adaptive Process Usage Analyzer!\033[0m\n";
            cout << "  \033[1;36mExiting...\033[0m\n\n";
            return 0;
        }

        default:
            cout << "\n  \033[1;31m[!] Invalid choice. Please enter 0-19.\033[0m\n";
            break;
        }
    }

    return 0;
}
