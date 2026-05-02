cmake_minimum_required(VERSION 3.10)
project(ProcessAnalyzerCLI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Threads
find_package(Threads REQUIRED)

# Executable
add_executable(analyzer_cli 
    main_cli.cpp
)

target_link_libraries(analyzer_cli PRIVATE Threads::Threads)
