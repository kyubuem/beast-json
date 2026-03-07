#pragma once
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/resource.h>

namespace bench {

inline size_t get_peak_rss_kb() {
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
#if defined(__APPLE__)
  return ru.ru_maxrss / 1024;
#else
  return ru.ru_maxrss;
#endif
}

// Read entire file into string
inline std::string read_file(const char *path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f.is_open()) {
    throw std::runtime_error(std::string("Failed to open file: ") + path);
  }
  std::streamsize size = f.tellg();
  f.seekg(0, std::ios::beg);
  std::string buffer(size, '\0');
  if (!f.read(&buffer[0], size)) {
    throw std::runtime_error(std::string("Failed to read file: ") + path);
  }
  return buffer;
}

// High precision timer
class Timer {
public:
  using clock = std::chrono::high_resolution_clock;
  using time_point = clock::time_point;

  void start() { start_ = clock::now(); }

  double elapsed_ns() const {
    auto end = clock::now();
    return std::chrono::duration<double, std::nano>(end - start_).count();
  }

  double elapsed_us() const { return elapsed_ns() / 1000.0; }

  double elapsed_ms() const { return elapsed_ns() / 1000000.0; }

private:
  time_point start_;
};

// Benchmark result
struct Result {
  std::string library;
  double parse_time_ns;
  double serialize_time_ns;
  bool correctness_check;

  void print() const {
    std::cout << std::left << std::setw(15) << library
              << " | Parse: " << std::right << std::setw(9) << std::fixed
              << std::setprecision(2) << (parse_time_ns / 1000.0) << " μs"
              << " | Serialize: " << std::setw(9) << std::fixed
              << std::setprecision(2) << (serialize_time_ns / 1000.0) << " μs"
              << " | Mem: " << std::setw(6) << get_peak_rss_kb() << " KB"
              << " | ✓ " << (correctness_check ? "PASS" : "FAIL") << "\n";
  }
};

// Print header
inline void print_header(const std::string &benchmark_name) {
  std::cout << "\n=== " << benchmark_name << " ===\n";
  std::cout << std::string(80, '-') << "\n";
}

// Print table header
inline void print_table_header() {
  std::cout << std::left << std::setw(20) << "Library" << " | Timings\n";
  std::cout << std::string(80, '-') << "\n";
}

} // namespace bench
