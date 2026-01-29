# Beast JSON (Extreme Optimization Edition)

Beast JSON is a high-performance, C++17 compliant JSON library designed for maximum throughput in parsing and serialization. It utilizes a tape-based architecture, raw pointer traversal, and hardware-specific optimizations (SWAR/SIMD).

## Performance Status
Current benchmarks on `twitter.json` (ARM64, M1/M2/M3):

| Operation | Speed | Note |
|:--- |:--- |:--- |
| **Serialization** | **~4.08 GB/s** | Best-in-class performance. |
| **Parsing** | **~1238 MB/s** | Peak achieved. Currently debugging a regression (~870 MB/s). |

> **Comparison**: Faster than RapidJSON (~970 MB/s) and nlohmann/json (~180 MB/s). Closing the gap with simdjson (~2800 MB/s) and yyjson (~3200 MB/s).

## Key Features
- **Tape Architecture**: Parses JSON into a contiguous 64-bit tape for cache-friendly traversal.
- **Zero-Check Parsing**: Pre-allocated buffers allow "scan-then-copy" string parsing without internal bounds checks.
- **Optimized Serialization**: Direct buffer writing with integer/float optimizations.
- **Hybrid SIMD/SWAR**: Uses ARM NEON for string scanning (in progress).

## Todo / Roadmap

### 1. Fix Parsing Regression
- **Issue**: Parsing speed dropped from 1238 MB/s to ~870 MB/s after refactoring `parse_number`.
- **Action**: Verify `parse_string` and `simd::skip_whitespace` integration. Re-validate the "Zero-Check" loop availability.

### 2. Int64 Support
- **Issue**: Currently all numbers are stored as `double`, causing potential precision loss for large 64-bit integers (e.g., Twitter IDs).
- **Action**: Implement `Type::Int64` (and `Type::Uint64`) to store integers natively on the tape.

### 3. Structural Unrolling
- **Issue**: `parse_array` and `parse_object` process one element at a time with function call overhead.
- **Action**: Unroll loops to check for multiple delimiters (`,`, `]`) or whitespace blocks at once.

### 4. Advanced SIMD
- Implement 128-bit SWAR for finding structural characters (`:`, `,`, `{`, `}`, `[`, `]`) to skip generic scalar loops.

## Build & Test
```bash
cd tests
# Compile comparative benchmark
g++ -std=c++17 -O3 -march=native -I.. cmp_benchmark.cpp libs/simdjson.cpp libs/yyjson.c -o cmp_benchmark
# Run
./cmp_benchmark twitter.json
```