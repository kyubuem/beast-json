#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

TEST(Unicode, Escapes) {
  // \u0041 -> 'A'
  // \u0024 -> '$'
  // \u20AC -> '€' (Euro Sign - 3 bytes UTF-8: E2 82 AC)
  std::string json = R"({"utf8": "\u0041\u0024\u20AC"})";

  tape::Document doc;
  tape::Parser p(doc, json.data(), json.size());
  ASSERT_TRUE(p.parse()) << "Parse Failed: " << p.parse().message;

  tape::TapeView root(doc, 0);
  std::string val(root["utf8"].get_string());

  EXPECT_EQ(val, "A$€");
}

TEST(Unicode, SurrogatePairs) {
  // Surrogate Pair Test: G Clef (U+1D11E) -> \uD834\uDD1E
  std::string json2 = R"({"music": "\uD834\uDD1E"})";
  tape::Document doc2;
  tape::Parser p2(doc2, json2.data(), json2.size());
  ASSERT_TRUE(p2.parse()) << "Surrogate Parse Failed";

  std::string music(tape::TapeView(doc2, 0)["music"].get_string());
  // U+1D11E in UTF-8: F0 9D 84 9E
  EXPECT_EQ(music, "\xF0\x9D\x84\x9E");
}
