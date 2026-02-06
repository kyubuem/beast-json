# Beast JSON Benchmark Results

## Overview
Performance comparison of `beast_json` against top JSON libraries in C++.

### Test Environment
- **Date**: 2026-02-05
- **Build**: Release (-O3)  
- **Compiler**: AppleClang 17.0
- **CPU**: Apple Silicon

---

## 1. File I/O Benchmark

**Test**: Parse `twitter.json` (616.7 KB) → Serialize → Verify output

| Library | Parse Time (μs) | Serialize Time (μs) | Correctness |
|---------|----------------|-------------------|-------------|
| beast_json | 1,933 | 733 | ⚠️ Format diff |
| nlohmann/json | 5,666 | 2,370 | ✓ |
| RapidJSON | 1,794 | 1,921 | ✓ |
| **simdjson** | **572** | N/A (parse-only) | ✓ |
| **yyjson** | **476** | **294** | ⚠️ Format diff |

**Winner**: 🏆 **yyjson** (476μs parse + 294μs serialize = **770μs total**)

---

## 2. Simple Struct Serialization

**Test**: `Person { string, int, string }` × 10,000 iterations (per-operation avg)

| Library | Deserialize (μs) | Serialize (μs) | Correctness |
|---------|-----------------|---------------|-------------|
| **beast_json** | 0.28 | **0.13** | ✓ |
| nlohmann/json | 0.81 | 1.06 | ✓ |
| RapidJSON | 1.12 | 1.07 | ✓ |
| **yyjson** | **0.11** | 0.15 | ✓ |

**Winner**: 🏆 **yyjson** (0.11μs parse) + **beast_json** (0.13μs serialize)

---

## 3. Complex Nested Struct

**Test**: `Company { string, vector<Person>, map<string,vector<int>>, optional<Address> }` × 1,000 iterations

| Library | Deserialize (μs) | Serialize (μs) | Correctness |
|---------|-----------------|---------------|-------------|
| **beast_json** | **0.87** | **0.75** | ⚠️ Reflection issue |
| nlohmann/json | 8.58 | 4.47 | ✓ |

*Note: RapidJSON and yyjson omitted due to excessive manual coding required*

**Winner**: 🏆 **beast_json** (10x faster than nlohmann, but needs correctness fix)

---

## 4. Glaze (C++23) Comparison

**Test**: Same complex `Company` struct × 1,000 iterations

| Library | Deserialize (μs) | Serialize (μs) | Correctness |
|---------|-----------------|---------------|-------------|
| beast_json (C++17) | 2.17 | 1.91 | ⚠️ |
| **Glaze (C++23)** | **1.23** | **0.65** | ✓ |

**Winner**: 🏆 **Glaze** (C++23 reflection gives 40% performance boost)

---

## Summary

### Key Findings

| Category | Fastest Library | Speed |
|----------|----------------|-------|
| **Large File Parse** | simdjson | 572 μs |
| **Large File Serialize** | yyjson | 294 μs |
| **Simple Struct Parse** | yyjson | 0.11 μs |
| **Simple Struct Serialize** | beast_json | 0.13 μs |
| **Complex Struct** | beast_json* | 0.87 μs parse (*with bugs) |
| **C++23 Reflection** | Glaze | 1.23 μs parse |

### beast_json Performance
- ✅ **Excellent**: 2-3x faster than nlohmann/json
- ✅ **Competitive**: Within 2-4x of yyjson/simdjson  
- ⚠️ **Issues**: Correctness failures on reflection-based serialization
- 🔧 **TODO**: Fix reflection macro for proper whitespace/formatting

### Recommendations
- **Raw Speed**: Use `yyjson` or `simdjson`
- **Ease of Use**: Use `beast_json` or `Glaze` (reflection-based)
- **Production**: Use `nlohmann/json` (correctness-first, slower but reliable)

---

## How to Run

```bash
# Build benchmarks
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBEAST_JSON_BUILD_TESTS=OFF
cmake --build build

# Run benchmarks
./build/benchmarks/bench_file_io
./build/benchmarks/bench_simple_struct
./build/benchmarks/bench_complex_struct
./build/benchmarks/bench_glaze  # Requires C++23 compiler
```

## Dependencies
All dependencies fetched automatically via CMake `FetchContent`:
- nlohmann/json v3.11.3
- RapidJSON (master)
- simdjson v3.10.1
- yyjson 0.10.0
- Glaze v2.9.5 (C++23)
