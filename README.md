# Beast JSON (Extreme Optimization Edition)

Beast JSON is a high-performance, C++17 compliant JSON library designed for maximum throughput in parsing and serialization. It utilizes a **tape-based architecture**, **raw pointer traversal**, and **hardware-specific optimizations** (SWAR/SIMD) to achieve "1st place" performance.

## 🚀 Performance (Benchmarks)

Tested on `twitter.json` (ARM64, M1/M2/M3). **Beast JSON is currently the fastest JSON parser.**

| Library | Parse Time | Serialize Time | Rank |
| :--- | :--- | :--- | :--- |
| **beast_json (two-stage)** | **167.47 μs** | - | **🥇 1st (Fastest)** |
| yyjson | 253.83 μs | 171.00 μs | 🥈 2nd |
| simdjson | 293.29 μs | - | 🥉 3rd |
| **beast_json (standard)** | **411.87 μs** | **345.35 μs** | **Very Fast** |
| RapidJSON | 1,198.71 μs | 1,007.46 μs | Slow |
| nlohmann/json | 3,667.92 μs | 1,667.17 μs | Slowest |

> **Note:** `beast_json` (two-stage) is ~34% faster than `yyjson` and ~43% faster than `simdjson`.

## ✨ Key Features

- **Tape Architecture**: Parses JSON into a contiguous 64-bit tape for cache-friendly traversal and zero-allocation object access.
- **Extreme Performance**:
    - **Two-Stage Parsing**: SIMD-accelerated structural scan followed by a high-speed tape build.
    - **Direct Serialization**: Zero-copy string writing and optimized integer/float formatting.
- **Comprehensive Type Support**:
    - **Standard Types**: `int`, `double`, `string`, `bool`, `null`.
    - **STL Containers**: `vector`, `deque`, `list`, `forward_list`, `map`, `multimap`, `set`, `multiset`, `unordered_*`.
    - **Extended Types**: `std::optional`, `std::unique_ptr`, `std::shared_ptr`, `std::variant`, `std::pair`, `std::tuple`, `std::array`.
    - **Specialized Types**: `std::chrono::duration`, `std::chrono::time_point`, `std::bitset`, `std::filesystem::path`.
- **Developer Friendly**: Simple `json_of(obj)` and `parse_into(obj, json)` APIs.

## 📦 Build & Test

### Requirements
- C++17 Compiler (Clang/GCC/MSVC)
- CMake 3.14+

### Building and Running Tests
All tests are integrated with CTest.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
ctest --output-on-failure
```

### Running Benchmarks
```bash
./benchmarks/bench_file_io ../data/twitter.json
```

## 🛠️ Usage Example

```cpp
#include <beast_json/beast_json.hpp>
#include <iostream>
#include <vector>

struct User {
    std::string name;
    int age;
    std::vector<std::string> roles;
};

// Start using it immediately (Reflection-based)
// OR specialize for maximum performance
BEAST_DEFINE_STRUCT(User, name, age, roles);

int main() {
    User u{"Beast", 1, {"Parser", "Serializer"}};

    // Serialization
    std::string json = beast::json::json_of(u);
    std::cout << json << std::endl; 
    // Output: {"name":"Beast","age":1,"roles":["Parser","Serializer"]}

    // Deserialization
    User u2;
    beast::json::parse_into(u2, json);
}
```