#include "beast_json/beast_json.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  using namespace beast::json;

  std::cout << "Testing Tape Features...\n";

  // Test 1: Large Integer (UInt64)
  {
    const char *json = "{\"id\": 18446744073709551615}"; // UINT64_MAX
    tape::Document doc;
    tape::Parser p(doc, json, std::strlen(json));

    auto res = p.parse();
    if (!res) {
      std::cerr << "Parse failed: " << res.message << "\n";
      return 1;
    }
    tape::TapeView root(doc);
    tape::TapeView id = root["id"];

    if (id.type() != tape::Type::UInt64) {
      std::cerr << "Test 1 Failed: Expected UInt64, got " << (int)id.type()
                << "\n";
      return 1;
    }
    if (id.get_uint64() != 18446744073709551615ULL) {
      std::cerr << "Test 1 Failed: Value mismatch\n";
      return 1;
    }
    std::cout << "Test 1 (UInt64 Max) Passed\n";
  }

  // Test 2: Array Iteration with UInt64 (Skip Check)
  {
    // [1, 18446744073709551615, 2]
    // 1 -> Int64 (1 slot + 1)
    // Big -> UInt64 (1 slot + 1) -> Total 2 slots
    // Array uses next_element_idx logic.
    const char *json = "[1, 18446744073709551615, 2]";
    tape::Document doc;
    tape::Parser p(doc, json, std::strlen(json));

    auto res = p.parse();
    if (!res) {
      std::cerr << "Parse 2 failed: " << res.message << " at offset "
                << res.offset << "\n";
      return 1;
    }
    tape::TapeView root(doc);

    int count = 0;
    uint64_t vals[3];
    for (auto v : root) {
      if (count < 3)
        vals[count] = v.get_uint64();
      count++;
    }

    if (count != 3) {
      std::cerr << "Test 2 Failed: Array iteration count " << count
                << " expected 3\n";
      return 1;
    }
    if (vals[1] != 18446744073709551615ULL) {
      std::cerr << "Test 2 Failed: Middle value mismatch\n";
      return 1;
    }
    if (vals[2] != 2) {
      std::cerr << "Test 2 Failed: Last value mismatch\n";
      return 1;
    }
    std::cout << "Test 2 (Array Skip) Passed\n";
  }

  // Test 3: Large Int64 (Negative)
  {
    const char *json = "{\"val\": -9223372036854775807}"; // INT64_MIN + 1
    tape::Document doc;
    tape::Parser p(doc, json, std::strlen(json));

    auto res = p.parse();
    if (!res) {
      std::cerr << "Parse 3 failed: " << res.message << "\n";
      return 1;
    }
    tape::TapeView root(doc);
    tape::TapeView val = root["val"];
    if (val.get_int64() != -9223372036854775807LL) {
      std::cerr << "Test 3 Failed: Negative Int64 mismatch\n";
      return 1;
    }
    std::cout << "Test 3 (Negative Int64) Passed\n";
  }

  std::cout << "All Tape tests passed!\n";
  return 0;
}
