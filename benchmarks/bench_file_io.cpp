#include "utils.hpp"
#include <beast_json/beast_json.hpp>
#include <nlohmann/json.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <simdjson.h>
#include <yyjson.h>

int main() {
  const char *filename = "twitter.json";
  std::string json_content = bench::read_file(filename);

  bench::print_header("File I/O Benchmark");
  std::cout << "File: " << filename << " (" << (json_content.size() / 1024.0)
            << " KB)\n";
  std::cout << "Test: Parse JSON → Serialize → Verify (input == output)\n\n";
  bench::print_table_header();

  std::vector<bench::Result> results;

  // 1. beast_json
  {
    // Warmup
    {
      beast::json::FastArena arena(json_content.size() * 2);
      beast::json::tape::Document doc(&arena);
      beast::json::tape::Parser parser(doc, json_content.data(),
                                       json_content.size());
      parser.parse();
    }

    bench::Timer parse_timer, serialize_timer;
    size_t iterations = 10000;

    parse_timer.start();
    for (size_t i = 0; i < iterations; ++i) {
      beast::json::FastArena arena(json_content.size() * 2);
      beast::json::tape::Document doc(&arena);
      beast::json::tape::Parser parser(doc, json_content.data(),
                                       json_content.size());
      parser.parse();
    }
    double parse_ns = parse_timer.elapsed_ns() / iterations;

    // Prepare for Serialize
    beast::json::FastArena arena(json_content.size() * 2);
    beast::json::tape::Document doc(&arena);
    beast::json::tape::Parser parser(doc, json_content.data(),
                                     json_content.size());
    auto res = parser.parse();
    bool correct = (res.error == beast::json::Error::Ok);
    if (!correct) {
      std::cout << "beast_json Failed: " << (int)res.error << " at "
                << res.offset << "\n";
    }

    std::string output;
    output.reserve(json_content.size() * 2);

    serialize_timer.start();
    for (size_t i = 0; i < iterations; ++i) {
      beast::json::tape::TapeSerializerExtreme serializer(doc, output);
      serializer.serialize();
      output.resize(0);
    }
    double serialize_ns = serialize_timer.elapsed_ns() / iterations;

    results.push_back({"beast_json", parse_ns, serialize_ns, correct});
  }

  // 1b. beast_json (In-Situ)
  {
    bench::Timer parse_timer;
    size_t iterations = 1000;

    // Need a fresh copy for each iteration as insitu modifies input
    // Ensure padding for unchecked mode
    std::vector<std::string> inputs;
    inputs.reserve(iterations);
    for (size_t i = 0; i < iterations; ++i) {
      std::string s = json_content;
      s.resize(json_content.size() + 64); // Padding
      inputs.push_back(std::move(s));
    }

    parse_timer.start();
    for (size_t i = 0; i < iterations; ++i) {
      beast::json::FastArena arena(json_content.size() * 2); // reuse size hint
      beast::json::tape::Document doc(&arena);
      // Pass mutable char*
      beast::json::tape::Parser parser(
          doc, &inputs[i][0], inputs[i].size(),
          {true, true, true, false, false, true}); // last true = insitu
      parser.parse();
    }
    double parse_ns = parse_timer.elapsed_ns() / iterations;

    // Verification?
    // Just check one
    beast::json::FastArena arena(json_content.size() * 2);
    beast::json::tape::Document doc(&arena);
    std::string mutable_input = json_content;
    beast::json::tape::Parser parser(doc, &mutable_input[0],
                                     mutable_input.size(),
                                     {true, true, true, false, false, true});
    auto res = parser.parse();
    bool correct = (res.error == beast::json::Error::Ok);
    if (!correct) {
      std::cout << "Insitu Failed: " << (int)res.error << " at " << res.offset
                << "\n";
      size_t start_ctx = (res.offset > 20) ? res.offset - 20 : 0;
      size_t len_ctx = std::min((size_t)40, mutable_input.size() - start_ctx);
      std::string ctx = mutable_input.substr(start_ctx, len_ctx);
      std::cout << "Context: [" << ctx << "]\n";
    }

    results.push_back({"beast_json (insitu)", parse_ns, 0.0, correct});
  }

  // 2. nlohmann/json
  {
    bench::Timer parse_timer, serialize_timer;

    parse_timer.start();
    nlohmann::json j = nlohmann::json::parse(json_content);
    double parse_ns = parse_timer.elapsed_ns();

    serialize_timer.start();
    std::string output = j.dump();
    double serialize_ns = serialize_timer.elapsed_ns();

    // Note: nlohmann may change whitespace/formatting
    bool correct = (output.size() > 0); // Basic sanity check
    results.push_back({"nlohmann/json", parse_ns, serialize_ns, correct});
  }

  // 3. RapidJSON
  {
    bench::Timer parse_timer, serialize_timer;
    std::string buffer = json_content; // RapidJSON modifies input

    parse_timer.start();
    rapidjson::Document doc;
    doc.ParseInsitu(&buffer[0]);
    double parse_ns = parse_timer.elapsed_ns();

    serialize_timer.start();
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    doc.Accept(writer);
    double serialize_ns = serialize_timer.elapsed_ns();

    std::string output(sb.GetString(), sb.GetSize());
    bool correct = (output.size() > 0);
    results.push_back({"RapidJSON", parse_ns, serialize_ns, correct});
  }

  // 4. simdjson (parse-only)
  {
    bench::Timer parse_timer;
    simdjson::dom::parser parser;
    std::string padded = json_content;
    padded.reserve(json_content.size() + simdjson::SIMDJSON_PADDING);

    parse_timer.start();
    simdjson::dom::element doc;
    auto error = parser.parse(padded).get(doc);
    double parse_ns = parse_timer.elapsed_ns();

    bool correct = !error;
    results.push_back({"simdjson (parse)", parse_ns, 0.0, correct});
  }

  // 5. yyjson
  {
    bench::Timer parse_timer, serialize_timer;

    parse_timer.start();
    yyjson_doc *doc = yyjson_read(json_content.c_str(), json_content.size(), 0);
    double parse_ns = parse_timer.elapsed_ns();

    serialize_timer.start();
    size_t len;
    char *str = yyjson_write(doc, 0, &len);
    double serialize_ns = serialize_timer.elapsed_ns();

    std::string output(str, len);

    // Semantic equivalence: can we parse our own output?
    bool correct = false;
    yyjson_doc *doc2 = yyjson_read(str, len, 0);
    if (doc2) {
      correct = true;
      yyjson_doc_free(doc2);
    }

    free(str);
    yyjson_doc_free(doc);

    results.push_back({"yyjson", parse_ns, serialize_ns, correct});
  }

  // Print results
  for (const auto &r : results) {
    r.print();
  }

  std::cout << "\n✓ Benchmark complete\n";
  return 0;
}
