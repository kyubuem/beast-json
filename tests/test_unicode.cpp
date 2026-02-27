#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

// The lazy parser stores strings as raw bytes from source. Unicode escape
// sequences (\uXXXX) are NOT decoded; they are stored and re-emitted as-is.
// dump() reproduces the exact source bytes (compact, no whitespace).

TEST(Unicode, EscapeRoundTrip) {
  // \u0041='A', \u0024='$', \u20AC=euro — stored raw, re-emitted unchanged
  // Use compact JSON (no spaces) since dump() outputs compact form.
  std::string json = R"({"utf8":"\u0041\u0024\u20AC"})";
  lazy::DocumentView doc;
  auto v = lazy::parse_reuse(doc, json);
  EXPECT_EQ(v.dump(), json);
}

TEST(Unicode, SurrogatePairRoundTrip) {
  // Surrogate pair \uD834\uDD1E (G Clef U+1D11E) stored as raw bytes
  std::string json = R"({"music":"\uD834\uDD1E"})";
  lazy::DocumentView doc;
  auto v = lazy::parse_reuse(doc, json);
  EXPECT_EQ(v.dump(), json);
}

TEST(Unicode, LiteralUtf8RoundTrip) {
  // Literal UTF-8 bytes round-trip unchanged
  std::string json = "{\"key\":\"\xE2\x82\xAC\"}"; // Euro sign as UTF-8
  lazy::DocumentView doc;
  auto v = lazy::parse_reuse(doc, json);
  EXPECT_EQ(v.dump(), json);
}

TEST(Unicode, MixedEscapesRoundTrip) {
  // Mix of escape sequences
  std::string json = R"({"a":"hello\nworld","b":"test"})";
  lazy::DocumentView doc;
  auto v = lazy::parse_reuse(doc, json);
  EXPECT_EQ(v.dump(), json);
}

TEST(Unicode, EmojiRoundTrip) {
  // 4-byte UTF-8 emoji (U+1F30D globe)
  std::string json = "{\"emoji\":\"\xF0\x9F\x8C\x8D\"}";
  lazy::DocumentView doc;
  auto v = lazy::parse_reuse(doc, json);
  EXPECT_EQ(v.dump(), json);
}
