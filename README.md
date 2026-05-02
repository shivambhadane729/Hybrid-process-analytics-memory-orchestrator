# Adaptive OS Memory Engine (Linux CLI)

A high-performance system telemetry and memory optimization engine migrated from Windows to Linux. It uses advanced data structures to analyze process "hotness" and provides an interactive terminal dashboard.

## Features
- **Real-time Monitoring**: Live process telemetry via \`/proc\`.
- **Hotness Scoring**: Multi-metric algorithm (CPU, RAM, Frequency, Recency).
- **Process Control**: Freeze (\`SIGSTOP\`) and Resume (\`SIGCONT\`) any process.
- **Priority Management**: Automatic \`nice\` value adjustment based on hotness.
- **Storage Tiering**: Simulated L1/L2/L3 memory hierarchy.
- **Memory Waste Detection**: Identifies idle processes consuming high RAM.

## Data Structures Used
- **Red-Black Tree & Skip List**: For sorted ranking.
- **Max Heap**: For Top-K hot processes.
- **Fenwick & Segment Trees**: For frequency and time-series analysis.
- **LRU Cache**: For recency tracking.

## Prerequisites
- **OS**: Ubuntu / Debian / Linux
- **Tools**: \`cmake\`, \`g++\`, \`make\`
- **Permissions**: Root (sudo) is required for process priority and freezing features.

## How to Build
\`\`\`bash
# Create build directory
mkdir -p build && cd build

# Configure and Build
cmake ..
make
\`\`\`

## How to Run
Run the analyzer with \`sudo\` to enable the prioritization and freezing features:
\`\`\`bash
sudo ./build/analyzer_cli
\`\`\`

## Interactive Controls
While the application is running, use these keys to switch views:
- **\`m\`**: Main Dashboard (Top processes)
- **\`w\`**: Memory Waste Analysis
- **\`d\`**: Data Structure Metrics
- **\`l\`**: Storage Layer Status
- **\`f\`**: Freeze a process (Prompts for PID)
- **\`r\`**: Resume a process (Prompts for PID)
- **\`q\`**: Quit the application
