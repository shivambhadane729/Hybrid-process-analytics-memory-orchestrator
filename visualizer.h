#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "data_structures.h"
#include "process_collector.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

using namespace std;

class Visualizer {
public:
    // ========================== STYLING ==========================
    static void printHeader(const string& title) {
        int width = 70;
        string border(width, '=');
        string padding((width - (int)title.size() - 4) / 2, ' ');

        cout << "\n";
        cout << "  " << border << "\n";
        cout << "  |" << padding << " " << title << " " << padding;
        if ((width - (int)title.size() - 4) % 2 != 0) cout << " ";
        cout << "|\n";
        cout << "  " << border << "\n";
    }

    static void printSubHeader(const string& title) {
        cout << "\n  --- " << title << " ---\n";
    }

    static void printSeparator() {
        cout << "  " << string(70, '-') << "\n";
    }

    static string classColor(const string& cls) {
        // ANSI color codes for terminal
        if (cls == "HOT")  return "\033[1;31m";  // Bold Red
        if (cls == "WARM") return "\033[1;33m";   // Bold Yellow
        if (cls == "COLD") return "\033[1;36m";   // Bold Cyan
        return "\033[0m";
    }

    static string resetColor() {
        return "\033[0m";
    }

    // ========================== MENU ==========================
    static void printMenu() {
        cout << "\n";
        cout << "  " << string(60, '=') << "\n";
        cout << "  |   ADAPTIVE PROCESS USAGE ANALYZER                     |\n";
        cout << "  |   Smart Memory Priority Recommendation System         |\n";
        cout << "  " << string(60, '=') << "\n";
        cout << "  |                                                        |\n";
        cout << "  |  [1]  Refresh / Collect Live Process Data              |\n";
        cout << "  |  [2]  Display All Processes (Hash Map)                 |\n";
        cout << "  |  [3]  Display Process Priority Table                   |\n";
        cout << "  |  [4]  Show HOT / WARM / COLD Classification            |\n";
        cout << "  |  [5]  Show Top K Priority Processes (Max Heap)         |\n";
        cout << "  |  [6]  Show Memory Waste Detection                      |\n";
        cout << "  |  [7]  Show Usage Time Report                           |\n";
        cout << "  |  [8]  Show Frequency Report (Fenwick Tree)             |\n";
        cout << "  |  [9]  Show Recency Report (LRU List)                   |\n";
        cout << "  |  [10] Show Smart Recommendations                       |\n";
        cout << "  |  [11] Show Time-Based Analysis (Segment Tree)          |\n";
        cout << "  |  [12] Show Skip List Ranking                           |\n";
        cout << "  |  [13] Show Red-Black Tree Sorted View                  |\n";
        cout << "  |  [14] Score Range Query (RB Tree)                      |\n";
        cout << "  |  [15] Generate Graph Data (for Python)                 |\n";
        cout << "  |  [0]  Exit                                             |\n";
        cout << "  |                                                        |\n";
        cout << "  " << string(60, '=') << "\n";
        cout << "\n  Enter your choice: ";
    }

    // ========================== DISPLAY FUNCTIONS ==========================

    // [2] Display all processes from Hash Map
    static void displayAllProcesses(const vector<ProcessData>& processes) {
        printHeader("ALL PROCESSES (Hash Map Storage)");

        cout << "\n  " << left
             << setw(25) << "Process"
             << setw(10) << "PID"
             << setw(14) << "Memory(MB)"
             << setw(10) << "CPU(%)"
             << setw(15) << "Active(min)"
             << "\n";
        printSeparator();

        for (auto& p : processes) {
            cout << "  " << left
                 << setw(25) << ProcessCollector::cleanName(p.name)
                 << setw(10) << p.pid
                 << setw(14) << fixed << setprecision(1) << p.memoryMB
                 << setw(10) << fixed << setprecision(2) << p.cpuPercent
                 << setw(15) << fixed << setprecision(1) << p.activeTimeMin
                 << "\n";
        }

        cout << "\n  Total processes: " << processes.size() << "\n";
    }

    // [3] Display priority table
    static void displayPriorityTable(vector<ProcessData> processes) {
        printHeader("PROCESS PRIORITY TABLE");

        // Sort by score descending
        sort(processes.begin(), processes.end(),
             [](const ProcessData& a, const ProcessData& b) {
                 return a.hotnessScore > b.hotnessScore;
             });

        cout << "\n  " << left
             << setw(25) << "Process"
             << setw(12) << "Score"
             << setw(10) << "Class"
             << setw(14) << "Memory(MB)"
             << setw(10) << "CPU(%)"
             << "\n";
        printSeparator();

        for (auto& p : processes) {
            cout << "  " << left
                 << setw(25) << ProcessCollector::cleanName(p.name)
                 << setw(12) << fixed << setprecision(1) << p.hotnessScore
                 << classColor(p.classification)
                 << setw(10) << p.classification
                 << resetColor()
                 << setw(14) << fixed << setprecision(1) << p.memoryMB
                 << setw(10) << fixed << setprecision(2) << p.cpuPercent
                 << "\n";
        }
    }

    // [4] Display HOT/WARM/COLD classification
    static void displayClassification(const vector<ProcessData>& hot,
                                       const vector<ProcessData>& warm,
                                       const vector<ProcessData>& cold) {
        printHeader("HOT / WARM / COLD CLASSIFICATION");

        // HOT
        cout << "\n  " << classColor("HOT") << "=== HOT (Score >= 70) ===" << resetColor() << "\n";
        if (hot.empty()) {
            cout << "  (none)\n";
        } else {
            for (auto& p : hot) {
                cout << "  " << classColor("HOT") << "  >> "
                     << ProcessCollector::cleanName(p.name)
                     << " (Score: " << fixed << setprecision(1) << p.hotnessScore << ")"
                     << resetColor() << "\n";
            }
        }

        // WARM
        cout << "\n  " << classColor("WARM") << "=== WARM (Score 40-70) ===" << resetColor() << "\n";
        if (warm.empty()) {
            cout << "  (none)\n";
        } else {
            for (auto& p : warm) {
                cout << "  " << classColor("WARM") << "  >> "
                     << ProcessCollector::cleanName(p.name)
                     << " (Score: " << fixed << setprecision(1) << p.hotnessScore << ")"
                     << resetColor() << "\n";
            }
        }

        // COLD
        cout << "\n  " << classColor("COLD") << "=== COLD (Score < 40) ===" << resetColor() << "\n";
        if (cold.empty()) {
            cout << "  (none)\n";
        } else {
            for (auto& p : cold) {
                cout << "  " << classColor("COLD") << "  >> "
                     << ProcessCollector::cleanName(p.name)
                     << " (Score: " << fixed << setprecision(1) << p.hotnessScore << ")"
                     << resetColor() << "\n";
            }
        }

        // Summary counts
        cout << "\n";
        printSeparator();
        cout << "  Summary: " << classColor("HOT") << hot.size() << " HOT" << resetColor()
             << " | " << classColor("WARM") << warm.size() << " WARM" << resetColor()
             << " | " << classColor("COLD") << cold.size() << " COLD" << resetColor() << "\n";
    }

    // [5] Display top K processes
    static void displayTopK(const vector<ProcessData>& topK, int k) {
        printHeader("TOP " + to_string(k) + " PRIORITY PROCESSES (Max Heap)");

        cout << "\n  " << left
             << setw(6)  << "Rank"
             << setw(25) << "Process"
             << setw(12) << "Score"
             << setw(10) << "Class"
             << "\n";
        printSeparator();

        for (int i = 0; i < (int)topK.size(); i++) {
            cout << "  " << left
                 << setw(6)  << (i + 1)
                 << setw(25) << ProcessCollector::cleanName(topK[i].name)
                 << setw(12) << fixed << setprecision(1) << topK[i].hotnessScore
                 << classColor(topK[i].classification)
                 << setw(10) << topK[i].classification
                 << resetColor()
                 << "\n";
        }
    }

    // [6] Display memory waste
    static void displayMemoryWaste(const vector<ProcessData>& waste) {
        printHeader("MEMORY WASTE DETECTION");

        if (waste.empty()) {
            cout << "\n  No significant memory waste detected!\n";
            return;
        }

        cout << "\n  High Memory + Low Usage Processes:\n\n";
        cout << "  " << left
             << setw(25) << "Process"
             << setw(14) << "Memory(MB)"
             << setw(12) << "Score"
             << setw(10) << "Status"
             << "\n";
        printSeparator();

        for (auto& p : waste) {
            string status = (p.cpuPercent < 0.5) ? "IDLE" : "LOW USE";
            cout << "  " << left
                 << setw(25) << ProcessCollector::cleanName(p.name)
                 << setw(14) << fixed << setprecision(1) << p.memoryMB
                 << setw(12) << fixed << setprecision(1) << p.hotnessScore
                 << "\033[1;31m" << setw(10) << status << resetColor()
                 << "\n";
        }

        // Total wasted memory
        double totalWaste = 0;
        for (auto& p : waste) totalWaste += p.memoryMB;
        cout << "\n  Total potentially recoverable memory: "
             << "\033[1;31m" << fixed << setprecision(1) << totalWaste << " MB" << resetColor() << "\n";
    }

    // [7] Display usage time report
    static void displayUsageTimeReport(vector<ProcessData> processes) {
        printHeader("USAGE TIME REPORT");

        sort(processes.begin(), processes.end(),
             [](const ProcessData& a, const ProcessData& b) {
                 return a.activeTimeMin > b.activeTimeMin;
             });

        cout << "\n  " << left
             << setw(25) << "Process"
             << setw(20) << "Active Time"
             << setw(30) << "Bar"
             << "\n";
        printSeparator();

        double maxTime = processes.empty() ? 1 : processes[0].activeTimeMin;
        if (maxTime < 1) maxTime = 1;

        for (auto& p : processes) {
            // Format time
            int hours = (int)(p.activeTimeMin / 60);
            int mins = (int)fmod(p.activeTimeMin, 60.0);
            string timeStr = to_string(hours) + "h " + to_string(mins) + "m";

            // Bar chart
            int barLen = (int)((p.activeTimeMin / maxTime) * 25);
            string bar(barLen, '#');

            cout << "  " << left
                 << setw(25) << ProcessCollector::cleanName(p.name)
                 << setw(20) << timeStr
                 << "\033[1;32m" << bar << resetColor()
                 << "\n";
        }
    }

    // [8] Display frequency report
    static void displayFrequencyReport(vector<ProcessData> processes) {
        printHeader("FREQUENCY REPORT (Fenwick Tree)");

        sort(processes.begin(), processes.end(),
             [](const ProcessData& a, const ProcessData& b) {
                 return a.focusCount > b.focusCount;
             });

        cout << "\n  " << left
             << setw(25) << "Process"
             << setw(15) << "Focus Count"
             << setw(30) << "Bar"
             << "\n";
        printSeparator();

        int maxCount = processes.empty() ? 1 : processes[0].focusCount;
        if (maxCount < 1) maxCount = 1;

        for (auto& p : processes) {
            int barLen = (int)((double)p.focusCount / maxCount * 25);
            string bar(barLen, '*');

            cout << "  " << left
                 << setw(25) << ProcessCollector::cleanName(p.name)
                 << setw(15) << p.focusCount
                 << "\033[1;35m" << bar << resetColor()
                 << "\n";
        }
    }

    // [9] Display recency report
    static void displayRecencyReport(const vector<ProcessData>& recency) {
        printHeader("RECENCY REPORT (LRU Doubly Linked List)");

        cout << "\n  " << left
             << setw(25) << "Process"
             << setw(20) << "Last Used"
             << setw(10) << "Class"
             << "\n";
        printSeparator();

        time_t now = time(nullptr);
        for (auto& p : recency) {
            double secAgo = difftime(now, p.lastUsedTime);
            string timeAgo;
            if (secAgo < 60) timeAgo = to_string((int)secAgo) + " sec ago";
            else if (secAgo < 3600) timeAgo = to_string((int)(secAgo / 60)) + " min ago";
            else timeAgo = to_string((int)(secAgo / 3600)) + " hr ago";

            cout << "  " << left
                 << setw(25) << ProcessCollector::cleanName(p.name)
                 << setw(20) << timeAgo
                 << classColor(p.classification)
                 << setw(10) << p.classification
                 << resetColor()
                 << "\n";
        }
    }

    // [10] Display smart recommendations
    static void displayRecommendations(const vector<ProcessData>& keepHigh,
                                        const vector<ProcessData>& deprioritize) {
        printHeader("SMART RECOMMENDATIONS");

        cout << "\n  " << "\033[1;32m" << "=== KEEP HIGH PRIORITY ===" << resetColor() << "\n";
        if (keepHigh.empty()) {
            cout << "  (none)\n";
        } else {
            for (auto& p : keepHigh) {
                cout << "  " << "\033[1;32m" << "  [+] "
                     << ProcessCollector::cleanName(p.name)
                     << " (Score: " << fixed << setprecision(1) << p.hotnessScore
                     << ", Mem: " << fixed << setprecision(0) << p.memoryMB << "MB)"
                     << resetColor() << "\n";
            }
        }

        cout << "\n  " << "\033[1;31m" << "=== DEPRIORITIZE / CLOSE ===" << resetColor() << "\n";
        if (deprioritize.empty()) {
            cout << "  (none)\n";
        } else {
            for (auto& p : deprioritize) {
                cout << "  " << "\033[1;31m" << "  [-] "
                     << ProcessCollector::cleanName(p.name)
                     << " (Score: " << fixed << setprecision(1) << p.hotnessScore
                     << ", Mem: " << fixed << setprecision(0) << p.memoryMB << "MB)"
                     << resetColor() << "\n";
            }

            // Calculate potential savings
            double savings = 0;
            for (auto& p : deprioritize) savings += p.memoryMB;
            cout << "\n  " << "\033[1;33m"
                 << "  Potential memory savings: " << fixed << setprecision(0) << savings << " MB"
                 << resetColor() << "\n";
        }
    }

    // [11] Display segment tree time analysis
    static void displayTimeAnalysis(const vector<ProcessData>& processes,
                                     const SegmentTree& segTree,
                                     const unordered_map<int, int>& pidToIndex) {
        printHeader("TIME-BASED ANALYSIS (Segment Tree)");

        if (processes.empty()) {
            cout << "\n  No data available.\n";
            return;
        }

        int n = (int)processes.size();

        // Divide into quartiles for analysis
        int q1 = n / 4;
        int q2 = n / 2;
        int q3 = 3 * n / 4;

        cout << "\n  Process Range Analysis:\n\n";

        double total = segTree.query(0, n - 1);
        cout << "  Total active time (all processes): "
             << fixed << setprecision(1) << total << " minutes\n\n";

        if (q1 > 0) {
            double usage1 = segTree.query(0, q1 - 1);
            cout << "  Quartile 1 (Processes 1-" << q1 << "): "
                 << fixed << setprecision(1) << usage1 << " min ("
                 << fixed << setprecision(1) << (total > 0 ? usage1/total*100 : 0) << "%)\n";
        }
        if (q2 > q1) {
            double usage2 = segTree.query(q1, q2 - 1);
            cout << "  Quartile 2 (Processes " << q1+1 << "-" << q2 << "): "
                 << fixed << setprecision(1) << usage2 << " min ("
                 << fixed << setprecision(1) << (total > 0 ? usage2/total*100 : 0) << "%)\n";
        }
        if (q3 > q2) {
            double usage3 = segTree.query(q2, q3 - 1);
            cout << "  Quartile 3 (Processes " << q2+1 << "-" << q3 << "): "
                 << fixed << setprecision(1) << usage3 << " min ("
                 << fixed << setprecision(1) << (total > 0 ? usage3/total*100 : 0) << "%)\n";
        }
        if (n > q3) {
            double usage4 = segTree.query(q3, n - 1);
            cout << "  Quartile 4 (Processes " << q3+1 << "-" << n << "): "
                 << fixed << setprecision(1) << usage4 << " min ("
                 << fixed << setprecision(1) << (total > 0 ? usage4/total*100 : 0) << "%)\n";
        }

        // Top 5 time consumers
        vector<ProcessData> sorted = processes;
        sort(sorted.begin(), sorted.end(),
             [](const ProcessData& a, const ProcessData& b) {
                 return a.activeTimeMin > b.activeTimeMin;
             });

        cout << "\n  Top 5 Time Consumers:\n";
        printSeparator();
        for (int i = 0; i < min(5, (int)sorted.size()); i++) {
            int hours = (int)(sorted[i].activeTimeMin / 60);
            int mins = (int)fmod(sorted[i].activeTimeMin, 60.0);
            cout << "  " << (i+1) << ". " << ProcessCollector::cleanName(sorted[i].name)
                 << " -> " << hours << "h " << mins << "m\n";
        }
    }

    // [12] Display skip list ranking
    static void displaySkipListRanking(const vector<ProcessData>& sorted) {
        printHeader("SKIP LIST RANKING");

        cout << "\n  " << left
             << setw(6)  << "Rank"
             << setw(25) << "Process"
             << setw(12) << "Score"
             << setw(10) << "Class"
             << "\n";
        printSeparator();

        for (int i = 0; i < (int)sorted.size(); i++) {
            cout << "  " << left
                 << setw(6)  << (i + 1)
                 << setw(25) << ProcessCollector::cleanName(sorted[i].name)
                 << setw(12) << fixed << setprecision(1) << sorted[i].hotnessScore
                 << classColor(sorted[i].classification)
                 << setw(10) << sorted[i].classification
                 << resetColor()
                 << "\n";
        }
    }

    // [13] Display RB tree sorted view
    static void displayRBTreeView(const vector<ProcessData>& sorted) {
        printHeader("RED-BLACK TREE SORTED VIEW");

        cout << "\n  " << left
             << setw(6)  << "Rank"
             << setw(25) << "Process"
             << setw(12) << "Score"
             << setw(10) << "Class"
             << setw(14) << "Memory(MB)"
             << "\n";
        printSeparator();

        for (int i = 0; i < (int)sorted.size(); i++) {
            cout << "  " << left
                 << setw(6)  << (i + 1)
                 << setw(25) << ProcessCollector::cleanName(sorted[i].name)
                 << setw(12) << fixed << setprecision(1) << sorted[i].hotnessScore
                 << classColor(sorted[i].classification)
                 << setw(10) << sorted[i].classification
                 << resetColor()
                 << setw(14) << fixed << setprecision(1) << sorted[i].memoryMB
                 << "\n";
        }
    }

    // [14] Score range query display
    static void displayScoreRange(const vector<ProcessData>& results, double low, double high) {
        printHeader("SCORE RANGE QUERY [" + to_string((int)low) + " - " + to_string((int)high) + "]");

        if (results.empty()) {
            cout << "\n  No processes found in this score range.\n";
            return;
        }

        cout << "\n  " << left
             << setw(25) << "Process"
             << setw(12) << "Score"
             << setw(10) << "Class"
             << "\n";
        printSeparator();

        for (auto& p : results) {
            cout << "  " << left
                 << setw(25) << ProcessCollector::cleanName(p.name)
                 << setw(12) << fixed << setprecision(1) << p.hotnessScore
                 << classColor(p.classification)
                 << setw(10) << p.classification
                 << resetColor()
                 << "\n";
        }

        cout << "\n  Found " << results.size() << " processes in range.\n";
    }

    // [15] Generate CSV data for Python graphs
    static void generateGraphData(const vector<ProcessData>& processes, const string& filename) {
        printHeader("GENERATING GRAPH DATA");

        ofstream file(filename);
        if (!file.is_open()) {
            cout << "\n  [ERROR] Could not create " << filename << "\n";
            return;
        }

        file << "Name,PID,MemoryMB,CPU,ActiveTimeMin,FocusCount,HotnessScore,Classification,LastUsedSecAgo,ForegroundDur,BackgroundDur\n";

        time_t now = time(nullptr);
        for (auto& p : processes) {
            double secAgo = difftime(now, p.lastUsedTime);
            file << ProcessCollector::cleanName(p.name) << ","
                 << p.pid << ","
                 << fixed << setprecision(2) << p.memoryMB << ","
                 << fixed << setprecision(2) << p.cpuPercent << ","
                 << fixed << setprecision(2) << p.activeTimeMin << ","
                 << p.focusCount << ","
                 << fixed << setprecision(2) << p.hotnessScore << ","
                 << p.classification << ","
                 << fixed << setprecision(0) << secAgo << ","
                 << fixed << setprecision(2) << p.foregroundDur << ","
                 << fixed << setprecision(2) << p.backgroundDur << "\n";
        }

        file.close();
        cout << "\n  [OK] Graph data saved to: " << filename << "\n";
        cout << "  Run: python graph_generator.py to generate graphs.\n";
    }
};

#endif // VISUALIZER_H
