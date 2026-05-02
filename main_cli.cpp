#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>
#include <algorithm>
#include "analyzer.h"

using namespace std;

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printHeader() {
    cout << "================================================================================\n";
    cout << "                ADAPTIVE OS MEMORY ENGINE - SYSTEM MONITOR                     \n";
    cout << "================================================================================\n";
    cout << left << setw(8) << "PID" 
         << setw(25) << "Process Name" 
         << setw(12) << "Mem (MB)" 
         << setw(10) << "CPU %" 
         << setw(12) << "Hotness" 
         << "Classification" << endl;
    cout << "--------------------------------------------------------------------------------\n";
}

int main() {
    Analyzer analyzer;

    while (true) {
        analyzer.collectAndStore();
        auto processes = analyzer.getAllProcesses();

        // Sort by hotness score for the display
        sort(processes.begin(), processes.end(), [](const ProcessData& a, const ProcessData& b) {
            return a.hotnessScore > b.hotnessScore;
        });

        clearScreen();
        printHeader();

        int count = 0;
        int hotCount = 0;
        int coldCount = 0;

        for (const auto& p : processes) {
            if (p.classification == "HOT") hotCount++;
            else if (p.classification == "COLD") coldCount++;

            if (count < 20) {
                cout << left << setw(8) << p.pid 
                     << setw(25) << Analyzer::cleanName(p.name).substr(0, 24)
                     << setw(12) << fixed << setprecision(1) << p.memoryMB 
                     << setw(10) << p.cpuPercent 
                     << setw(12) << p.hotnessScore 
                     << p.classification << endl;
                count++;
            }
        }

        cout << "\n[Storage Layers Status]\n";
        cout << "HOT (L1 Cache/SSD): " << hotCount << " processes\n";
        cout << "COLD (L3 HDD/Disk): " << coldCount << " processes\n";

        cout << "\nUpdating in 2 seconds... (Ctrl+C to exit)\n";
        this_thread::sleep_for(chrono::seconds(2));
    }

    return 0;
}
