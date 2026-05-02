#ifndef LINUX_PROCESS_COLLECTOR_H
#define LINUX_PROCESS_COLLECTOR_H

#include "i_process_collector.h"
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <signal.h>
#include <cstring>
#include <iostream>
#include <algorithm>

class LinuxProcessCollector : public IProcessCollector {
public:
    std::vector<ProcessData> collectLiveProcesses() override {
        std::vector<ProcessData> processes;
        DIR* dir = opendir("/proc");
        if (!dir) return processes;

        struct dirent* entry;
        long uptime = getUptime();
        long clk_tck = sysconf(_SC_CLK_TCK);

        while ((entry = readdir(dir)) != nullptr) {
            // Check if directory name is a number (PID)
            if (entry->d_type == DT_DIR && isNumber(entry->d_name)) {
                int pid = std::stoi(entry->d_name);
                if (pid == 0) continue;

                ProcessData pd;
                if (parseProcessStat(pid, pd, uptime, clk_tck)) {
                    processes.push_back(pd);
                }
            }
        }
        closedir(dir);
        return processes;
    }

    void applySystemPriority(int pid, const std::string& classification) override {
        int priority = 0;
        if (classification == "HOT") priority = -10;      // Higher priority (negative nice)
        else if (classification == "WARM") priority = 0; // Default
        else if (classification == "COLD") priority = 10; // Lower priority (positive nice)

        // Note: setting negative priority (boosting) usually requires root or CAP_SYS_NICE
        if (setpriority(PRIO_PROCESS, pid, priority) == -1) {
            // Silently fail if no permission, or log it
        }
    }

    bool freezeProcess(int pid) override {
        // SIGSTOP freezes a process so it consumes no CPU but stays in memory
        return kill(pid, SIGSTOP) == 0;
    }

    bool resumeProcess(int pid) override {
        // SIGCONT resumes a frozen process
        return kill(pid, SIGCONT) == 0;
    }

private:
    bool isNumber(const std::string& s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
    }

    long getUptime() {
        struct sysinfo info;
        if (sysinfo(&info) == 0) return info.uptime;
        return 0;
    }

    bool parseProcessStat(int pid, ProcessData& pd, long uptime, long clk_tck) {
        std::string path = "/proc/" + std::to_string(pid) + "/stat";
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::string line;
        std::getline(file, line);
        
        // Format of /proc/[pid]/stat:
        // pid (comm) state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt utime stime cutime cstime priority nice num_threads itrealvalue starttime vsize rss rsslim ...
        
        std::size_t openParen = line.find('(');
        std::size_t closeParen = line.find(')');
        if (openParen == std::string::npos || closeParen == std::string::npos) return false;

        pd.pid = pid;
        pd.name = line.substr(openParen + 1, closeParen - openParen - 1);

        std::string rest = line.substr(closeParen + 2);
        std::stringstream ss(rest);
        std::vector<std::string> tokens;
        std::string token;
        while (ss >> token) tokens.push_back(token);

        if (tokens.size() < 20) return false;

        // 0: state, 1: ppid, ..., 7: flags, 8: minflt, 10: majflt, 12: utime, 13: stime, 17: priority, 18: nice, 19: num_threads, 21: starttime, 22: vsize, 23: rss
        
        pd.pageFaultCount = std::stoul(tokens[8]) + std::stoul(tokens[10]); // minflt + majflt
        
        unsigned long long utime = std::stoull(tokens[12]);
        unsigned long long stime = std::stoull(tokens[13]);
        unsigned long long starttime = std::stoull(tokens[21]);
        
        double total_time = (double)(utime + stime) / clk_tck;
        double seconds = uptime - (starttime / clk_tck);
        
        if (seconds > 0) {
            pd.cpuPercent = (total_time / seconds) * 100.0;
        } else {
            pd.cpuPercent = 0;
        }

        pd.activeTimeMin = seconds / 60.0;
        pd.startTime = time(nullptr) - (long)seconds;

        pd.memoryMB = (std::stoull(tokens[23]) * getpagesize()) / (1024.0 * 1024.0); // RSS in pages
        
        // peakWorkingSetKB isn't directly in /proc/pid/stat, could get from /proc/pid/status VmPeak
        // For now, use VSize / 1024
        pd.peakWorkingSetKB = std::stoull(tokens[22]) / 1024.0;

        // Simulate focus count and recency for now (as native focus detection is complex)
        pd.focusCount = 1;
        pd.lastUsedTime = time(nullptr);

        return true;
    }
};

#endif // LINUX_PROCESS_COLLECTOR_H
