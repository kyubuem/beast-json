#!/bin/bash
# Profiling helper script for macOS Instruments

echo "=== Beast JSON Profiling Guide ==="
echo ""
echo "Step 1: Build with debug symbols"
echo "  cmake -B build-profile -DCMAKE_BUILD_TYPE=RelWithDebInfo"
echo "  cmake --build build-profile --target bench_file_io -j8"
echo ""
echo "Step 2: Run Instruments Time Profiler"
echo "  instruments -t 'Time Profiler' -D profile_results.trace \\"
echo "    build-profile/benchmarks/bench_file_io"
echo ""
echo "Step 3: Open trace file"
echo "  open profile_results.trace"
echo ""
echo "Alternative: Run in Xcode Instruments GUI"
echo "  1. Open Xcode"
echo "  2. Xcode -> Open Developer Tool -> Instruments"
echo "  3. Choose 'Time Profiler' template"
echo "  4. Choose target: build-profile/benchmarks/bench_file_io"
echo "  5. Click Record"
echo ""
echo "=== Quick Profile (command line) ==="
echo ""

# Check if we should run
if [ "$1" == "run" ]; then
    echo "Building with profiling symbols..."
    cmake -B build-profile -DCMAKE_BUILD_TYPE=RelWithDebInfo
    cmake --build build-profile --target bench_file_io -j8
    
    echo ""
    echo "Running Instruments (this may take a moment)..."
    instruments -t 'Time Profiler' \
        -D beast_json_profile.trace \
        build-profile/benchmarks/bench_file_io
    
    echo ""
    echo "Opening results..."
    open beast_json_profile.trace
else
    echo "To run profiling automatically, use: $0 run"
fi
