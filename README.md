# Beast JSON (Extreme Optimization Edition)

**Beast JSON** is a high-performance, C++20 compliant JSON library designed for maximum throughput in parsing and serialization. Born as an **AI-Only Generated Library**, it represents an experimental playground for absolute, zero-cost extreme optimizations.

## 🤖 An AI-Generated Library
This entire project—including its architecture, SIMD/SWAR optimizations, test suites, and CMake build systems—was autonomously designed and written by Advanced Agentic AI (Antigravity ecosystem, Claude, and Gemini). The goal is to push the boundaries of what AI can achieve in low-level, high-performance C++ systems programming, proving that AI can compete with, and potentially beat, the world's fastest human-written libraries.

## 🌟 Our Vision
Our vision is simple: **To become the #1 fastest JSON library in the world.**

We are actively hunting down the performance records set by legendary C++ libraries like `yyjson` and `simdjson`. "Beast JSON" stands for our relentless, beast-like pursuit of CPU cycles, cache-line efficiency, and branchless perfection. There is no compromise; we are aiming for the absolute top.

## 🚀 Performance Benchmarks (Phase 30)

Tested on `twitter.json` (631KB) on Apple Silicon (ARM64 M-series).
We recently broke the 240μs barrier by combining **TapeNode memory-layout optimizations** and a **3-instruction NEON Whitespace Scanner**.

| Library | Parse Time | Parse Speed | Rank |
| :--- | :--- | :--- | :--- |
| yyjson | ~170 μs | ~3.7 GB/s | 🥇 1st |
| **beast_json (Current Phase 30)** | **236 μs** | **~2.6 GB/s** | **🥈 2nd** |
| simdjson | ~290 μs | ~2.1 GB/s | 🥉 3rd |
| RapidJSON | ~1,198 μs | ~520 MB/s | Slow |
| nlohmann/json | ~3,667 μs | ~170 MB/s | Slowest |

> **Current Status:** Beast JSON is now **~18% faster than simdjson** and is steadily closing the gap with `yyjson`. We are currently trailing the world record by only ~66μs.

## ✨ Key Features

- **Tape-Based Lazy DOM Architecture**: Parses JSON into a contiguous 16-byte `TapeNode` array for L1-cache friendly traversal and zero-allocation object access.
- **Extreme Hardware Optimization**:
    - **SWAR / NEON SIMD**: Vectorized whitespace skipping (3-instruction NEON fallback) and 24-byte SWAR cascade string parsing.
    - **Double-Pump Structural Traversal**: Inline consumption of structural tokens (`,`, `:`, `}`) bypassing standard `switch` overhead.
    - **Zero-Copy Serialization**: Direct pointer manipulation into string buffers with state-machine context tracking.
- **Developer Friendly**: Drop-in single header design (`beast_json.hpp`) with simple `.get_array()`, `.get_object()`, and `.find("key")` accessors.

## 📦 Build & Test

### Requirements
- C++20 Compiler (Clang 10+ / GCC 10+)
- CMake 3.14+
- ARM64 (Apple M-series or Linux ARM) naturally preferred for NEON paths, though x86-64 is fully supported.

### Building and Running Tests
```bash
mkdir build-release && cd build-release
cmake .. -DBEAST_JSON_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build .
ctest --output-on-failure
```

### Running Benchmarks
We include comparative benching against `yyjson` and `simdjson`.
```bash
./benchmarks/bench_lazy_dom ../twitter.json
```

## 🛠️ Usage Example

Since Phase 19, `beast-json` operates as a high-speed, zero-allocation Lazy DOM.

```cpp
#include <beast_json/beast_json.hpp>
#include <iostream>
#include <string_view>

using namespace beast::json;

int main() {
    std::string_view json = R"({"name": "Beast", "speed": 236, "tags": ["AI", "Fast"]})";

    // 1. Parsing: Zero-allocation document view
    lazy::DocumentView doc(json);
    lazy::Value root = lazy::parse_reuse(doc, json);

    if (doc.error_code != lazy::Error::Ok) {
        std::cerr << "Parse failed!" << std::endl;
        return 1;
    }

    // 2. Accessing (Zero-copy AST traversal)
    if (root.is_object()) {
        std::cout << "Name: " << root.find("name").get_string() << "\n";
        std::cout << "Speed: " << root.find("speed").get_int64() << " us\n";
        
        lazy::Value tags = root.find("tags");
        if (tags.is_array()) {
            std::cout << "First Tag: " << tags.get_array().at(0).get_string() << "\n";
        }
    }

    // 3. Serialization (Direct zero-copy dump)
    std::string out = root.dump();
    std::cout << "Serialized: " << out << std::endl;

    return 0;
}
```