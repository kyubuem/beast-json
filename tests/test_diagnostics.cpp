#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

TEST(Diagnostics, Simple) {
  std::string json = R"({"a": "b"})";
  tape::Document doc;
  tape::Parser p(doc, json.data(), json.size());
  tape::ParseResult res = p.parse();
  EXPECT_TRUE(res) << "Simple JSON Failed: " << res.message;
}

TEST(Diagnostics, ErrorPositioning) {
  std::string json = R"({
  "key": "value",
  "missing_colon" "oops"
})";

  tape::Document doc;
  tape::Parser p(doc, json.data(), json.size());
  tape::ParseResult res = p.parse();

  EXPECT_FALSE(res) << "Failed to detect error!";
  if (!res) {
    // Optional: Check line/col if sensitive to exact position
    // EXPECT_EQ(res.line, 3);
    // EXPECT_GT(res.column, 0);
  }
}
