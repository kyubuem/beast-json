#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// Include Beast JSON (Target)
#include "../beast_json.hpp"

// Include Competitors
#include "libs/json.hpp" // nlohmann
#include "libs/rapidjson/include/rapidjson/document.h"
#include "libs/rapidjson/include/rapidjson/stringbuffer.h"
#include "libs/rapidjson/include/rapidjson/writer.h"
#include "libs/simdjson.h"
#include "libs/yyjson.h"

using namespace std::chrono;

struct BenchmarkResult {
  std::string name;
  double parse_mb_s;
  double serialize_mb_s;
};

std::string read_file(const char *filename) {
  std::ifstream f(filename, std::ios::binary | std::ios::ate);
  if (!f)
    return "";
  auto size = f.tellg();
  std::string s(size, '\0');
  f.seekg(0);
  f.read(&s[0], size);
  return s;
}

void print_header() {
  std::cout << "| Library | Parse (MB/s) | Serialize (MB/s) |\n";
  std::cout << "| :--- | :--- | :--- |\n";
}

void print_row(const BenchmarkResult &r) {
  std::cout << "| " << std::left << std::setw(20) << r.name << " | "
            << std::fixed << std::setprecision(2) << r.parse_mb_s << " | "
            << r.serialize_mb_s << " |\n";
}

// Beast JSON - Tape (Optimized)
BenchmarkResult bench_beast_tape(const std::string &json, int iters) {
  beast::json::tape::Document tape_doc;

  // Warmup & Parse
  auto t1 = high_resolution_clock::now();
  for (int i = 0; i < iters; ++i) {
    tape_doc.tape.clear();
    tape_doc.string_buffer.clear();
    beast::json::tape::Parser parser(tape_doc, json.data(), json.size());
    parser.parse();
  }
  auto t2 = high_resolution_clock::now();
  double duration = duration_cast<microseconds>(t2 - t1).count();
  double parse_mb_s =
      (json.size() * iters / (1024.0 * 1024.0)) / (duration / 1000000.0);

  // Serialization
  std::string out_str;
  out_str.reserve(json.size());
  auto t3 = high_resolution_clock::now();
  for (int i = 0; i < iters; ++i) {
    out_str.clear();
    beast::json::tape::TapeSerializer serializer(tape_doc, out_str);
    serializer.serialize();
  }
  auto t4 = high_resolution_clock::now();
  double ser_duration = duration_cast<microseconds>(t4 - t3).count();
  double ser_mb_s =
      (json.size() * iters / (1024.0 * 1024.0)) / (ser_duration / 1000000.0);

  return {"Beast (Tape)", parse_mb_s, ser_mb_s};
}

// SimdJSON (DOM)
BenchmarkResult bench_simdjson(const std::string &json, int iters) {
  simdjson::dom::parser parser;
  std::string json_padded = json;
  json_padded.reserve(json.size() + simdjson::SIMDJSON_PADDING);

  double total_parse = 0;
  for (int i = 0; i < iters; ++i) {
    auto t1 = high_resolution_clock::now();
    simdjson::dom::element doc;
    auto error = parser.parse(json_padded).get(doc);
    auto t2 = high_resolution_clock::now();
    total_parse += duration_cast<microseconds>(t2 - t1).count();
    if (error)
      std::cerr << "Simd error\n";
  }

  // SimdJSON is mostly parse-only (DOM). Printing is possible but not
  // "serialize" in same sense? Actually doc.get_string() or similar? For now
  // 0.0 unless we want to minify. simdjson::minify(parser.parse(json_padded),
  // output_string); is possible.

  return {"simdjson (DOM)",
          (json.size() * iters / (1024.0 * 1024.0)) / (total_parse / 1000000.0),
          0.0};
}

// yyjson
BenchmarkResult bench_yyjson(const std::string &json, int iters) {
  double total_parse = 0;
  double total_ser = 0;

  for (int i = 0; i < iters; ++i) {
    auto t1 = high_resolution_clock::now();
    yyjson_doc *doc = yyjson_read(json.c_str(), json.size(), 0);
    auto t2 = high_resolution_clock::now();
    total_parse += duration_cast<microseconds>(t2 - t1).count();

    auto t3 = high_resolution_clock::now();
    char *s = yyjson_write(doc, 0, NULL);
    auto t4 = high_resolution_clock::now();
    total_ser += duration_cast<microseconds>(t4 - t3).count();

    free(s);
    yyjson_doc_free(doc);
  }

  double mb = json.size() / 1024.0 / 1024.0;
  return {"yyjson", (mb * iters * 1000000) / total_parse,
          (mb * iters * 1000000) / total_ser};
}

// RapidJSON
BenchmarkResult bench_rapidjson(const std::string &json, int iters) {
  double total_parse = 0;
  double total_ser = 0;
  std::string buf;

  for (int i = 0; i < iters; ++i) {
    buf = json;
    auto t1 = high_resolution_clock::now();
    rapidjson::Document d;
    d.ParseInsitu(&buf[0]);
    auto t2 = high_resolution_clock::now();
    total_parse += duration_cast<microseconds>(t2 - t1).count();

    auto t3 = high_resolution_clock::now();
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    auto t4 = high_resolution_clock::now();
    total_ser += duration_cast<microseconds>(t4 - t3).count();
  }

  double mb = json.size() / 1024.0 / 1024.0;
  return {"RapidJSON (InSitu)", (mb * iters * 1000000) / total_parse,
          (mb * iters * 1000000) / total_ser};
}

// nlohmann/json
BenchmarkResult bench_nlohmann(const std::string &json, int iters) {
  double total_parse = 0;
  double total_ser = 0;

  for (int i = 0; i < iters; ++i) {
    auto t1 = high_resolution_clock::now();
    nlohmann::json j = nlohmann::json::parse(json);
    auto t2 = high_resolution_clock::now();
    total_parse += duration_cast<microseconds>(t2 - t1).count();

    auto t3 = high_resolution_clock::now();
    std::string s = j.dump();
    auto t4 = high_resolution_clock::now();
    total_ser += duration_cast<microseconds>(t4 - t3).count();
  }

  double mb = json.size() / 1024.0 / 1024.0;
  return {"nlohmann/json", (mb * iters * 1000000) / total_parse,
          (mb * iters * 1000000) / total_ser};
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Usage: ./cmp_benchmark <json_file>\n";
    return 1;
  }

  std::string json = read_file(argv[1]);
  if (json.empty()) {
    std::cerr << "Failed to read file or empty.\n";
    return 1;
  }

  std::cout << "Benchmarking on " << argv[1] << " (" << json.size() / 1024.0
            << " KB)\n";
  int iters = 100;

  std::cout << "Running... wait...\n";
  auto r_tape = bench_beast_tape(json, iters);
  auto r_simd = bench_simdjson(json, iters);
  auto r_yy = bench_yyjson(json, iters);
  auto r_rapid = bench_rapidjson(json, iters);
  auto r_nlohmann = bench_nlohmann(json, iters);

  std::cout << "\nResults:\n";
  print_header();
  print_row(r_tape);
  print_row(r_simd);
  print_row(r_yy);
  print_row(r_rapid);
  print_row(r_nlohmann);

  return 0;
}
