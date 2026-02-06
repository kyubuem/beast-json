#include "utils.hpp"
#include <beast_json/beast_json.hpp>
#include <nlohmann/json.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <simdjson.h>
#include <vector>
#include <yyjson.h>

struct FileResult {
  std::string filename;
  size_t size_bytes;
  double beast_parse_us;
  double beast_serialize_us;
  double yyjson_parse_us;
  double yyjson_serialize_us;
  double simdjson_parse_us;
};

int main() {
  std::vector<const char *> files = {
      "../data/small.json", "../data/test_large.json",
      "../data/test_uint64.json",
      "../data/test_simple.json", // 617KB
      "twitter.json"              // 631KB
  };

  std::vector<FileResult> results;

  for (const char *file : files) {
    try {
      std::string json_content = bench::read_file(file);
      FileResult r;
      r.filename = file;
      r.size_bytes = json_content.size();

      // beast_json
      {
        bench::Timer parse_timer, serialize_timer;

        parse_timer.start();
        beast::json::FastArena arena(json_content.size() * 2);
        beast::json::tape::Document doc(&arena);
        beast::json::tape::Parser parser(doc, json_content.data(),
                                         json_content.size());
        parser.parse();
        r.beast_parse_us = parse_timer.elapsed_us();

        std::string output;
        output.reserve(json_content.size());
        serialize_timer.start();
        beast::json::tape::TapeSerializerExtreme serializer(doc, output);
        serializer.serialize();
        r.beast_serialize_us = serialize_timer.elapsed_us();
      }

      // yyjson
      {
        bench::Timer parse_timer, serialize_timer;

        parse_timer.start();
        yyjson_doc *doc =
            yyjson_read(json_content.c_str(), json_content.size(), 0);
        r.yyjson_parse_us = parse_timer.elapsed_us();

        serialize_timer.start();
        size_t len;
        char *str = yyjson_write(doc, 0, &len);
        r.yyjson_serialize_us = serialize_timer.elapsed_us();

        free(str);
        yyjson_doc_free(doc);
      }

      // simdjson
      {
        bench::Timer parse_timer;
        simdjson::dom::parser parser;
        std::string padded = json_content;
        padded.reserve(json_content.size() + simdjson::SIMDJSON_PADDING);

        parse_timer.start();
        simdjson::dom::element doc;
        auto error = parser.parse(padded).get(doc);
        r.simdjson_parse_us = parse_timer.elapsed_us();
      }

      results.push_back(r);

    } catch (const std::exception &e) {
      std::cerr << "Skipping " << file << ": " << e.what() << "\n";
    }
  }

  // Print results
  std::cout << "\n=== Performance Gap Analysis ===\n\n";
  std::cout << std::left << std::setw(25) << "File" << std::right
            << std::setw(12) << "Size" << std::setw(15) << "beast Parse"
            << std::setw(15) << "yyjson Parse" << std::setw(15) << "simdjson"
            << std::setw(12) << "Gap\n";
  std::cout << std::string(100, '-') << "\n";

  for (const auto &r : results) {
    double gap = r.beast_parse_us / r.yyjson_parse_us;
    std::string size_str = r.size_bytes < 1024
                               ? std::to_string(r.size_bytes) + "B"
                               : std::to_string(r.size_bytes / 1024) + "KB";

    std::cout << std::left << std::setw(25) << r.filename << std::right
              << std::setw(12) << size_str << std::setw(13) << std::fixed
              << std::setprecision(2) << r.beast_parse_us << " μs"
              << std::setw(13) << r.yyjson_parse_us << " μs" << std::setw(13)
              << r.simdjson_parse_us << " μs" << std::setw(10) << gap << "x\n";
  }

  std::cout << "\n=== Serialization Comparison ===\n\n";
  std::cout << std::left << std::setw(25) << "File" << std::right
            << std::setw(15) << "beast Ser" << std::setw(15) << "yyjson Ser"
            << std::setw(12) << "Gap\n";
  std::cout << std::string(70, '-') << "\n";

  for (const auto &r : results) {
    double gap = r.beast_serialize_us / r.yyjson_serialize_us;
    std::cout << std::left << std::setw(25) << r.filename << std::right
              << std::setw(13) << std::fixed << std::setprecision(2)
              << r.beast_serialize_us << " μs" << std::setw(13)
              << r.yyjson_serialize_us << " μs" << std::setw(10) << gap
              << "x\n";
  }

  return 0;
}
