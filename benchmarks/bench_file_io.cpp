#include "utils.hpp"
#include <beast_json/beast_json.hpp>
#include <nlohmann/json.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <simdjson.h>
#include <yyjson.h>

void verify_beast(const std::string &original_json,
                  const std::string &beast_serialized,
                  const std::string &lib_name) {
  try {
    auto j1 = nlohmann::json::parse(original_json);
    auto j2 = nlohmann::json::parse(beast_serialized);
    if (j1 != j2) {
      std::cout << "Verify Failed for " << lib_name
                << ": Semantically different.\n";
      std::cout << "Output SNIP: " << beast_serialized.substr(0, 100)
                << "...\n";
    }
  } catch (const std::exception &e) {
    std::cout << "Verify Failed for " << lib_name << ": " << e.what() << "\n";
    std::cout << "Output SNIP: " << beast_serialized.substr(0, 100) << "...\n";
  }
}

int main(int argc, char **argv) {
  const char *filename = (argc > 1) ? argv[1] : "twitter.json";
  std::string json_content = bench::read_file(filename);

  if (json_content.empty())
    return 1;

  const size_t iterations = 200; // Faster for debugging
  bench::print_header("File I/O Benchmark (Strict)");
  std::cout << "File: " << filename << " (" << (json_content.size() / 1024.0)
            << " KB)\n";
  bench::print_table_header();

  std::vector<bench::Result> results;

  // 1. beast_json
  {
    beast::json::FastArena arena(json_content.size() * 50);
    bench::Timer pt, st;
    pt.start();
    for (size_t i = 0; i < iterations; ++i) {
//       beast::json::tape::Document doc(&arena);
//       beast::json::tape::Parser parser(doc, json_content.data(),
arena.reset();
      beast::json::parse(json_content, &arena);
    }
    double p_ns = pt.elapsed_ns() / iterations;
//     beast::json::tape::Document doc(&arena);
//     beast::json::tape::Parser parser(doc, json_content.data(),
arena.reset();
      beast::json::parse(json_content, &arena);

    std::string ser_out;
    ser_out.reserve(json_content.size() * 2);
    st.start();
    for (size_t i = 0; i < iterations; ++i) {
      ser_out.resize(0);
      beast::json::parse(json_content, &arena).dump();
    }
    double s_ns = st.elapsed_ns() / iterations;

    std::string v_out = beast::json::parse(json_content, &arena).dump();

    bool ok = false;
    try {
      auto j1 = nlohmann::json::parse(json_content);
      auto j2 = nlohmann::json::parse(v_out);
      ok = (j1 == j2);
    } catch (...) {
    }
    if (!ok)
      verify_beast(json_content, v_out, "beast_json");
    results.push_back({"beast_json", p_ns, s_ns, ok});
  }

  // 1b. beast_json (insitu)
  {
    beast::json::FastArena arena(json_content.size() * 50);
    std::string input_copy = json_content;
    input_copy.resize(json_content.size() + 64);
    bench::Timer pt, st;
    pt.start();
    for (size_t i = 0; i < iterations; ++i) {
//       beast::json::tape::Document doc(&arena);
//       beast::json::tape::Parser parser(
arena.reset();
      beast::json::parse(json_content, &arena);
    }
    double p_ns = pt.elapsed_ns() / iterations;
//     beast::json::tape::Document doc(&arena);
//     beast::json::tape::Parser parser(
arena.reset();
      beast::json::parse(json_content, &arena);

    std::string ser_out;
    ser_out.reserve(json_content.size() * 2);
    st.start();
    for (size_t i = 0; i < iterations; ++i) {
      ser_out.resize(0);
      beast::json::parse(json_content, &arena).dump();
    }
    double s_ns = st.elapsed_ns() / iterations;

    std::string v_out = beast::json::parse(json_content, &arena).dump();
    bool ok = false;
    try {
      auto j1 = nlohmann::json::parse(json_content);
      auto j2 = nlohmann::json::parse(v_out);
      ok = (j1 == j2);
    } catch (...) {
    }
    if (!ok)
      verify_beast(json_content, v_out, "beast_json (insitu)");
    results.push_back({"beast_json (insitu)", p_ns, s_ns, ok});
  }

  // 1c. beast_json (nitro)
  {
    beast::json::FastArena arena(json_content.size() * 50);
    bench::Timer pt, st;
    pt.start();
    for (size_t i = 0; i < iterations; ++i) {
//       beast::json::tape::Document doc(&arena);
//       beast::json::tape::Parser parser(
arena.reset();
      beast::json::parse(json_content, &arena);
    }
    double p_ns = pt.elapsed_ns() / iterations;
//     beast::json::tape::Document doc(&arena);
//     beast::json::tape::Parser parser(
arena.reset();
      beast::json::parse(json_content, &arena);

    std::string ser_out;
    ser_out.reserve(json_content.size() * 2);
    st.start();
    for (size_t i = 0; i < iterations; ++i) {
      ser_out.resize(0);
      beast::json::parse(json_content, &arena).dump();
    }
    double s_ns = st.elapsed_ns() / iterations;

    std::string v_out = beast::json::parse(json_content, &arena).dump();
    bool ok = false;
    try {
      auto j1 = nlohmann::json::parse(json_content);
      auto j2 = nlohmann::json::parse(v_out);
      ok = (j1 == j2);
    } catch (...) {
    }
    if (!ok)
      verify_beast(json_content, v_out, "beast_json (nitro)");
    results.push_back({"beast_json (nitro)", p_ns, s_ns, ok});
  }

  // 2. nlohmann
  {
    bench::Timer pt, st;
    pt.start();
    for (size_t i = 0; i < iterations; ++i) {
      nlohmann::json j = nlohmann::json::parse(json_content);
    }
    double p_ns = pt.elapsed_ns() / iterations;
    nlohmann::json j = nlohmann::json::parse(json_content);
    st.start();
    for (size_t i = 0; i < iterations; ++i) {
      std::string s = j.dump();
    }
    double s_ns = st.elapsed_ns() / iterations;
    results.push_back({"nlohmann/json", p_ns, s_ns, true});
  }

  // 3. yyjson
  {
    bench::Timer pt, st;
    pt.start();
    for (size_t i = 0; i < iterations; ++i) {
      yyjson_doc *d = yyjson_read(json_content.c_str(), json_content.size(), 0);
      yyjson_doc_free(d);
    }
    double p_ns = pt.elapsed_ns() / iterations;
    yyjson_doc *d = yyjson_read(json_content.c_str(), json_content.size(), 0);
    st.start();
    for (size_t i = 0; i < iterations; ++i) {
      size_t l;
      char *s = yyjson_write(d, 0, &l);
      free(s);
    }
    double s_ns = st.elapsed_ns() / iterations;
    yyjson_doc_free(d);
    results.push_back({"yyjson", p_ns, s_ns, true});
  }

  for (const auto &r : results)
    r.print();
  return 0;
}
