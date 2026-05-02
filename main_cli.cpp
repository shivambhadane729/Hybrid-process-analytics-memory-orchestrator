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
int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

void clearScreen() {
    system("clear");
}

void printMenu(char currentMode) {
    cout << "\n[M]AIN | [W]ASTE | [D]ATA STRUCT | [L]AYERS | [F]REEZE | [R]ESUME | [Q]UIT\n";
    cout << "Current Mode: " << (currentMode == 'm' ? "Dashboard" : (currentMode == 'w' ? "Waste" : (currentMode == 'd' ? "DS" : (currentMode == 'l' ? "Layers" : (currentMode == 'f' ? "Freeze" : "Resume"))))) << " | Press key\n";
}

int main() {
    Analyzer analyzer;
    char mode = 'm';

    while (mode != 'q') {
        analyzer.collectAndStore();
        auto allProcs = analyzer.getAllProcesses();
        clearScreen();

        if (mode == 'm') {
            cout << "========================== SYSTEM DASHBOARD ==========================\n";
            sort(allProcs.begin(), allProcs.end(), [](const ProcessData& a, const ProcessData& b) {
                return a.hotnessScore > b.hotnessScore;
            });
            cout << left << setw(8) << "PID" << setw(20) << "NAME" << setw(10) << "MEM" << setw(10) << "CPU" << "CLASS" << endl;
            int count = 0;
            for (auto& p : allProcs) {
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
            int hot = 0, warm = 0, cold = 0;
            for (auto& p : allProcs) {
                if (p.classification == "HOT") hot++;
                else if (p.classification == "WARM") warm++;
                else if (p.classification == "COLD") cold++;
            }
            cout << "L1 CACHE (HOT): " << hot << " processes\n";
            cout << "L2 SSD (WARM):  " << warm << " processes\n";
            cout << "L3 DISK (COLD): " << cold << " processes\n";
        }
        else if (mode == 'f' || mode == 'r') {
            cout << "========================== PROCESS CONTROL ===========================\n";
            cout << "Enter PID to " << (mode == 'f' ? "FREEZE (SIGSTOP)" : "RESUME (SIGCONT)") << ": ";
            
            // Temporary switch to blocking mode for input
            int pid;
            if (cin >> pid) {
                bool success = (mode == 'f' ? analyzer.freezeProcess(pid) : analyzer.resumeProcess(pid));
                if (success) cout << "\n[SUCCESS] Signal sent to PID " << pid << endl;
                else cout << "\n[ERROR] Failed to send signal to PID " << pid << " (Check sudo?)" << endl;
                this_thread::sleep_for(chrono::seconds(1));
            }
            mode = 'm'; // Return to main dashboard
            cin.clear();
        }

        printMenu(mode);
        
        // Wait for input with timeout
        for(int i=0; i<20; i++) { // 2 second refresh
            if (kbhit()) {
                char in = getchar();
                if (in == 'm' || in == 'w' || in == 'd' || in == 'l' || in == 'q' || in == 'f' || in == 'r') mode = in;
                break;
            }
            usleep(100000); // 0.1s
        }
    }

    return 0;
}
