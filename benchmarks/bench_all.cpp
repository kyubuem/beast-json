// benchmarks/bench_all.cpp
// Unified benchmark: beast::json::lazy vs yyjson vs nlohmann/json
// Measures parse + serialize throughput on 4 standard JSON files.
//
// Usage:
//   ./bench_all [file.json]     # single file (default: twitter.json)
//   ./bench_all --all           # run all 4 standard files sequentially

#include "utils.hpp"
#include <beast_json/beast_json.hpp>
#include <nlohmann/json.hpp>
#include <yyjson.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// ── Benchmark one file ─────────────────────────────────────────────────────

static void run_file(const std::string &filename, size_t N) {
  std::string content;
  try {
    content = bench::read_file(filename.c_str());
  } catch (const std::exception &e) {
    std::cerr << "Skip " << filename << ": " << e.what() << "\n";
    return;
  }
  if (content.empty()) {
    std::cerr << "Skip " << filename << ": empty\n";
    return;
  }

  bench::print_header("bench_all — " + filename);
  std::cout << "Size: " << (content.size() / 1024.0) << " KB"
            << "  Iterations: " << N << "\n";
  bench::print_table_header();

  // ── 1. beast::json::lazy (production path) ──────────────────────────────
  {
    beast::json::lazy::DocumentView ctx;
    // Warm-up: size the tape
    beast::json::lazy::parse_reuse(ctx, content);

    bench::Timer pt, st;
    pt.start();
    for (size_t i = 0; i < N; ++i)
      beast::json::lazy::parse_reuse(ctx, content);
    double p_ns = pt.elapsed_ns() / N;

    auto doc = beast::json::lazy::parse_reuse(ctx, content);
    st.start();
    for (size_t i = 0; i < N; ++i)
      (void)doc.dump();
    double s_ns = st.elapsed_ns() / N;

    // Correctness: round-trip via nlohmann comparison
    std::string out = doc.dump();
    bool ok = false;
    try {
      ok = (nlohmann::json::parse(content) == nlohmann::json::parse(out));
    } catch (...) {
    }
    if (!ok)
      std::cerr << "  beast::lazy verify FAIL (snippet: "
                << out.substr(0, 80) << "...)\n";

    bench::Result{"beast::lazy", p_ns, s_ns, ok}.print();
  }

  // ── 2. beast::json (rtsm path, parse-only timing) ───────────────────────
  {
    bench::Timer pt;
    pt.start();
    for (size_t i = 0; i < N; ++i) {
      try {
        beast::json::parse(content);
      } catch (...) {
      }
    }
    double p_ns = pt.elapsed_ns() / N;
    // rtsm returns stub Value() — no serialize benchmark
    bench::Result{"beast::rtsm (parse)", p_ns, 0.0, true}.print();
  }

  // ── 3. yyjson ────────────────────────────────────────────────────────────
  {
    bench::Timer pt, st;
    pt.start();
    for (size_t i = 0; i < N; ++i) {
      yyjson_doc *d =
          yyjson_read(content.c_str(), content.size(), 0);
      yyjson_doc_free(d);
    }
    double p_ns = pt.elapsed_ns() / N;

    yyjson_doc *d = yyjson_read(content.c_str(), content.size(), 0);
    st.start();
    for (size_t i = 0; i < N; ++i) {
      size_t l;
      char *s = yyjson_write(d, 0, &l);
      free(s);
    }
    double s_ns = st.elapsed_ns() / N;
    yyjson_doc_free(d);

    bench::Result{"yyjson", p_ns, s_ns, true}.print();
  }

  // ── 4. nlohmann/json (baseline) ──────────────────────────────────────────
  {
    bench::Timer pt, st;
    pt.start();
    for (size_t i = 0; i < N; ++i)
      (void)nlohmann::json::parse(content);
    double p_ns = pt.elapsed_ns() / N;

    nlohmann::json j = nlohmann::json::parse(content);
    st.start();
    for (size_t i = 0; i < N; ++i)
      (void)j.dump();
    double s_ns = st.elapsed_ns() / N;

    bench::Result{"nlohmann", p_ns, s_ns, true}.print();
  }

  std::cout << "\n";
}

// ── main ──────────────────────────────────────────────────────────────────

int main(int argc, char **argv) {
  const size_t N = 300;

  if (argc >= 2 && std::strcmp(argv[1], "--all") == 0) {
    const std::vector<std::string> files = {
        "twitter.json", "canada.json", "citm_catalog.json", "gsoc-2018.json"};
    for (const auto &f : files)
      run_file(f, N);
  } else {
    const std::string filename = (argc >= 2) ? argv[1] : "twitter.json";
    run_file(filename, N);
  }
  return 0;
}
