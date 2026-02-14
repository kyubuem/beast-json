#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string_view>

using namespace beast::json;

class TrailingCommas : public ::testing::Test {
protected:
  bool parse_json(std::string_view json) {
    beast::json::tape::Document doc;
    beast::json::tape::ParseOptions opts;
    opts.allow_trailing_commas = true;
    beast::json::tape::Parser p(doc, json.data(), json.size(), opts);
    return p.parse();
  }
};

TEST_F(TrailingCommas, Array) {
  std::string arr = "[1, 2, 3, ]";
  EXPECT_TRUE(parse_json(arr)) << "Array trailing comma rejected";
}

TEST_F(TrailingCommas, Object) {
  std::string obj = R"({
        "a": 1,
        "b": 2, 
    })";
  EXPECT_TRUE(parse_json(obj)) << "Object trailing comma rejected";
}
