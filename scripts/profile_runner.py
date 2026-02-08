#!/usr/bin/env python3
"""
Simple profiling benchmark that runs longer for better Instruments data
"""

import sys
import json
from pathlib import Path

# Add parent to path for beast_json import if needed
sys.path.insert(0, str(Path(__file__).parent.parent / 'build' / 'benchmarks'))

def main():
    import subprocess
    
    # Read twitter.json
    json_file = Path(__file__).parent.parent / 'build' / 'benchmarks' / 'twitter.json'
    
    if not json_file.exists():
        print(f"Error: {json_file} not found")
        return 1
    
    with open(json_file, 'r') as f:
        json_content = f.read()
    
    print(f"Loaded {len(json_content)} bytes of JSON")
    print("Running 1000 iterations for profiling...")
    
    # Import the C++ module would go here, but we'll use subprocess instead
    # Run bench_file_io many times
    bench_path = Path(__file__).parent.parent / 'build' / 'benchmarks' / 'bench_file_io'
    
    for i in range(1000):
        if i % 100 == 0:
            print(f"Iteration {i}...")
        
        result = subprocess.run(
            [str(bench_path)],
            capture_output=True,
            cwd=bench_path.parent,
            timeout=5
        )
    
    print("Done!")
    return 0

if __name__ == '__main__':
    exit(main())
