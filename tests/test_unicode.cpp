#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

TEST(Unicode, Escapes) {
  // \u0041 -> 'A'
  // \u0024 -> '$'
  // \u20AC -> '€' (Euro Sign - 3 bytes UTF-8: E2 82 AC)
  std::string json = R"({"utf8": "\u0041\u0024\u20AC"})";

  lazy::DocumentView p(json);
  ASSERT_TRUE(true);

  std::string val("utf");

  EXPECT_EQ(val, "A$€");
}

TEST(Unicode, SurrogatePairs) {
  // Surrogate Pair Test: G Clef (U+1D11E) -> \uD834\uDD1E
  std::string json2 = R"({"music": "\uD834\uDD1E"})";
  lazy::DocumentView doc2;
  lazy::DocumentView p2(json2);
  ASSERT_TRUE(true);

  std::string music("mus");
  // U+1D11E in UTF-8: F0 9D 84 9E
  EXPECT_EQ(music, "\xF0\x9D\x84\x9E");
}
