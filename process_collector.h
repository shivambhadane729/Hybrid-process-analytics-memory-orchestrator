#ifndef PROCESS_COLLECTOR_H
#define PROCESS_COLLECTOR_H

#include "data_structures.h"
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <ctime>
#include <unordered_set>

using namespace std;

class ProcessCollector {
public:
    // Collect live process data from Windows
    static vector<ProcessData> collectLiveProcesses() {
        vector<ProcessData> processes;
        unordered_set<string> seenNames; // To aggregate by name

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            cout << "[ERROR] Failed to create process snapshot.\n";
            return processes;
        }

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe)) {
            do {
                ProcessData pd;

                // Convert wide char to string
                #ifdef UNICODE
                    char name[260];
                    wcstombs(name, pe.szExeFile, 260);
                    pd.name = string(name);
                #else
                    pd.name = string(pe.szExeFile);
                #endif

                pd.pid = pe.th32ProcessID;

                // Skip system idle process
                if (pd.pid == 0) continue;

                // Get memory info
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pd.pid);
                if (hProcess) {
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                        pd.memoryMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
                    }

                    // Get process times
                    FILETIME ftCreation, ftExit, ftKernel, ftUser;
                    if (GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
                        // Convert creation time to time_t
                        ULARGE_INTEGER uli;
                        uli.LowPart = ftCreation.dwLowDateTime;
                        uli.HighPart = ftCreation.dwHighDateTime;
                        // Convert from Windows FILETIME (100-ns intervals since 1601) to Unix time
                        pd.startTime = (time_t)((uli.QuadPart - 116444736000000000ULL) / 10000000ULL);

                        // Calculate active time
                        time_t now = time(nullptr);
                        pd.activeTimeMin = difftime(now, pd.startTime) / 60.0;
                        if (pd.activeTimeMin < 0) pd.activeTimeMin = 0;

                        // CPU time from kernel + user time
                        ULARGE_INTEGER kernelTime, userTime;
                        kernelTime.LowPart = ftKernel.dwLowDateTime;
                        kernelTime.HighPart = ftKernel.dwHighDateTime;
                        userTime.LowPart = ftUser.dwLowDateTime;
                        userTime.HighPart = ftUser.dwHighDateTime;

                        double totalCpuSec = (kernelTime.QuadPart + userTime.QuadPart) / 10000000.0;
                        double elapsedSec = difftime(now, pd.startTime);
                        if (elapsedSec > 0) {
                            pd.cpuPercent = (totalCpuSec / elapsedSec) * 100.0;
                        }
                    }

                    CloseHandle(hProcess);
                }

                // Simulated metrics (OS doesn't directly provide these easily)
                pd.lastUsedTime = time(nullptr) - (rand() % 3600); // Simulate
                pd.focusCount = 1 + (rand() % 20);
                pd.foregroundDur = pd.activeTimeMin * (0.3 + (rand() % 50) / 100.0);
                pd.backgroundDur = pd.activeTimeMin - pd.foregroundDur;
                if (pd.backgroundDur < 0) pd.backgroundDur = 0;

                // Skip duplicates by name (keep the one with highest memory)
                if (seenNames.count(pd.name)) {
                    // Find and merge
                    for (auto& existing : processes) {
                        if (existing.name == pd.name) {
                            existing.memoryMB += pd.memoryMB;
                            existing.focusCount += pd.focusCount;
                            if (pd.cpuPercent > existing.cpuPercent)
                                existing.cpuPercent = pd.cpuPercent;
                            break;
                        }
                    }
                } else {
                    seenNames.insert(pd.name);
                    processes.push_back(pd);
                }

            } while (Process32Next(hSnapshot, &pe));
        }

        CloseHandle(hSnapshot);
        return processes;
    }

    // Clean process name for display
    static string cleanName(const string& name) {
        string clean = name;
        // Remove .exe extension for display
        size_t pos = clean.rfind(".exe");
        if (pos == string::npos) pos = clean.rfind(".EXE");
        if (pos != string::npos) clean = clean.substr(0, pos);
        // Capitalize first letter
        if (!clean.empty()) clean[0] = toupper(clean[0]);
        return clean;
    }
};

#endif // PROCESS_COLLECTOR_H
