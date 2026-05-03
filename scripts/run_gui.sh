#!/bin/bash

# Configuration
BINARY="./analyzer_gui"
LIB_PATH="/lib/x86_64-linux-gnu"
LIBC_PATH="$LIB_PATH/libc.so.6"
PTHREAD_PATH="$LIB_PATH/libpthread.so.0"

# Check if binary exists
if [ ! -f "$BINARY" ]; then
    echo "Error: $BINARY not found. Please build the project first."
    exit 1
fi

# Run with LD_PRELOAD to resolve snap/host library conflicts
echo "Launching Hybrid Process Analytics Memory Orchestrator GUI..."
LD_PRELOAD="$LIBC_PATH $PTHREAD_PATH" $BINARY
