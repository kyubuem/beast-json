#include "include/beast_json/beast_json.hpp"
#include <chrono>
#include <iostream>

int main() {
  using namespace beast::json;

  // Test 1: Zero-copy string (no escapes)
  const char *json1 = R"({"name":"HelloWorld","age":42})";

  tape::FastArena arena1;
  tape::Document doc1(&arena1);
  tape::Parser parser1(doc1, json1, strlen(json1));

  auto start = std::chrono::high_resolution_clock::now();
  parser1.parse();
  auto end = std::chrono::high_resolution_clock::now();

  std::cout << "Parse (no escapes): "
            << std::chrono::duration<double, std::micro>(end - start).count()
            << " μs\n";

  auto val = doc1.root();
  if (val.is_object()) {
    auto obj = val.as_object();
    auto name = obj["name"];
    std::cout << "Name: " << name.as_string() << "\n";
  }

  // Test 2: String with escapes
  const char *json2 = R"({"text":"Hello\\nWorld\\t!"})";

  tape::FastArena arena2;
  tape::Document doc2(&arena2);
  tape::Parser parser2(doc2, json2, strlen(json2));

  start = std::chrono::high_resolution_clock::now();
  parser2.parse();
  end = std::chrono::high_resolution_clock::now();

  std::cout << "Parse (with escapes): "
            << std::chrono::duration<double, std::micro>(end - start).count()
            << " μs\n";

  auto val2 = doc2.root();
  if (val2.is_object()) {
    auto obj = val2.as_object();
    auto text = obj["text"];
    std::cout << "Text length: " << text.as_string().size() << "\n";
  }

  std::cout << "✅ NEON optimizations compiled successfully!\n";

  return 0;
}
