#ifndef FAULT_MONITOR_H
#define FAULT_MONITOR_H

#include "data_structures.h"
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace std;

// ========================== FAULT MONITOR ==========================
// Uses real Windows API data (PageFaultCount, PeakWorkingSetSize,
// PagefileUsage) already collected in ProcessData to prove that
// COLD processes are indeed swapping to disk more than HOT ones.
// ===================================================================
class FaultMonitor {
public:
    struct FaultSummary {
        string classification;  // HOT / WARM / COLD
        int processCount;
        unsigned long totalFaults;
        double avgFaults;
        double totalPagefileKB;
        double avgPagefileKB;
        double totalPeakWSKB;
        double avgPeakWSKB;
    };

    // Analyze page faults grouped by classification
    static vector<FaultSummary> analyzeByClassification(const vector<ProcessData>& processes) {
        // Group by classification
        vector<ProcessData> hot, warm, cold;
        for (auto& p : processes) {
            if (p.classification == "HOT") hot.push_back(p);
            else if (p.classification == "WARM") warm.push_back(p);
            else cold.push_back(p);
        }

        vector<FaultSummary> summaries;
        auto buildSummary = [](const vector<ProcessData>& group, const string& cls) -> FaultSummary {
            FaultSummary fs;
            fs.classification = cls;
            fs.processCount = (int)group.size();
            fs.totalFaults = 0;
            fs.totalPagefileKB = 0;
            fs.totalPeakWSKB = 0;
            for (auto& p : group) {
                fs.totalFaults += p.pageFaultCount;
                fs.totalPagefileKB += p.pagefileUsageKB;
                fs.totalPeakWSKB += p.peakWorkingSetKB;
            }
            fs.avgFaults = group.empty() ? 0 : (double)fs.totalFaults / group.size();
            fs.avgPagefileKB = group.empty() ? 0 : fs.totalPagefileKB / group.size();
            fs.avgPeakWSKB = group.empty() ? 0 : fs.totalPeakWSKB / group.size();
            return fs;
        };

        summaries.push_back(buildSummary(hot, "HOT"));
        summaries.push_back(buildSummary(warm, "WARM"));
        summaries.push_back(buildSummary(cold, "COLD"));
        return summaries;
    }

    // Get top N processes by page fault count
    static vector<ProcessData> getTopFaulters(const vector<ProcessData>& processes, int n = 10) {
        vector<ProcessData> sorted = processes;
        sort(sorted.begin(), sorted.end(),
             [](const ProcessData& a, const ProcessData& b) {
                 return a.pageFaultCount > b.pageFaultCount;
             });
        if ((int)sorted.size() > n) sorted.resize(n);
        return sorted;
    }

    // Compute correlation between page faults and hotness score
    // Returns Pearson correlation coefficient in [-1, 1]
    static double faultScoreCorrelation(const vector<ProcessData>& processes) {
        if (processes.size() < 2) return 0.0;
        int n = (int)processes.size();
        double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumY2 = 0;
        for (auto& p : processes) {
            double x = (double)p.pageFaultCount;
            double y = p.hotnessScore;
            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumX2 += x * x;
            sumY2 += y * y;
        }
        double denom = sqrt((n * sumX2 - sumX * sumX) * (n * sumY2 - sumY * sumY));
        if (denom == 0) return 0.0;
        return (n * sumXY - sumX * sumY) / denom;
    }

    // Get processes sorted by pagefile usage (most swapped)
    static vector<ProcessData> getMostSwapped(const vector<ProcessData>& processes, int n = 10) {
        vector<ProcessData> sorted = processes;
        sort(sorted.begin(), sorted.end(),
             [](const ProcessData& a, const ProcessData& b) {
                 return a.pagefileUsageKB > b.pagefileUsageKB;
             });
        if ((int)sorted.size() > n) sorted.resize(n);
        return sorted;
    }
};

#endif // FAULT_MONITOR_H
