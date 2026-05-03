#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>
#include <algorithm>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "analyzer.h"

using namespace std;

// Non-blocking key press detection for Linux
int getKey() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    // Set to non-blocking to check for key
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    return ch;
}

void clearScreen() {
    system("clear");
}

void printMenu(char currentMode) {
    cout << "\n================================================================================\n";
    cout << "[ENTER] REFRESH | [M]AIN | [W]ASTE | [D]ATA STRUCTURES | [L]AYERS | [Q]UIT\n";
    cout << "Current Mode: " << (currentMode == 'm' ? "Dashboard" : (currentMode == 'w' ? "Waste" : (currentMode == 'd' ? "Data Structures" : "Layers"))) << endl;
    cout << "================================================================================\n";
}

int main() {
    Analyzer analyzer;
    char mode = 'm';
    bool needsRefresh = true;

    while (mode != 'q') {
        if (needsRefresh) {
            analyzer.collectAndStore();
            clearScreen();
            
            if (mode == 'm') {
                cout << "========================== HYBRID PROCESS ANALYTICS DASHBOARD =========================\n";
                auto processes = analyzer.getAllProcesses();
                sort(processes.begin(), processes.end(), [](const ProcessData& a, const ProcessData& b) {
                    return a.hotnessScore > b.hotnessScore;
                });
                cout << left << setw(8) << "PID" << setw(20) << "NAME" << setw(10) << "MEM" << setw(10) << "CPU" << "CLASS" << endl;
                int count = 0;
                for (auto& p : processes) {
                    if (count++ >= 15) break;
                    cout << left << setw(8) << p.pid << setw(20) << Analyzer::cleanName(p.name).substr(0,19) 
                         << setw(10) << fixed << setprecision(1) << p.memoryMB << setw(10) << p.cpuPercent << p.classification << endl;
                }
            } 
            else if (mode == 'w') {
                cout << "========================== MEMORY WASTE ANALYSIS =====================\n";
                auto waste = analyzer.getMemoryWaste();
                if (waste.empty()) cout << "No significant memory waste detected (Cold + >100MB).\n";
                else {
                    cout << left << setw(8) << "PID" << setw(25) << "NAME" << "WASTED MEMORY" << endl;
                    for (auto& p : waste) {
                        cout << left << setw(8) << p.pid << setw(25) << Analyzer::cleanName(p.name) << p.memoryMB << " MB" << endl;
                    }
                }
            }
            else if (mode == 'd') {
                cout << "========================== DATA STRUCTURES VIEW ======================\n";
                cout << "RB-Tree Size: " << analyzer.getRBTree().size() << endl;
                cout << "Skip-List Size: " << analyzer.getSkipList().size() << endl;
                cout << "LRU Cache Size: " << analyzer.getLRUList().size() << endl;
                cout << "\nTop Process by RB-Tree Ranking:\n";
                auto top = analyzer.getSortedRBTree();
                if (!top.empty()) cout << " -> " << top[0].name << " (Score: " << top[0].hotnessScore << ")\n";
            }
            else if (mode == 'l') {
                cout << "========================== STORAGE LAYERS ============================\n";
                auto hot = analyzer.getByClassification("HOT");
                auto warm = analyzer.getByClassification("WARM");
                auto cold = analyzer.getByClassification("COLD");
                cout << "L1 CACHE (HOT): " << hot.size() << " processes\n";
                cout << "L2 SSD (WARM):  " << warm.size() << " processes\n";
                cout << "L3 DISK (COLD): " << cold.size() << " processes\n";
            }
            printMenu(mode);
            needsRefresh = false;
        }

        int in = getKey();
        if (in != EOF) {
            char c = (char)in;
            if (c == '\n' || c == '\r') {
                needsRefresh = true;
            } else if (c == 'm' || c == 'w' || c == 'd' || c == 'l' || c == 'q') {
                mode = (c == 'q') ? 'q' : c;
                needsRefresh = true;
            }
        }
        usleep(50000); // Reduce CPU usage while waiting for input
    }
    return 0;
}
