#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string_view>

using namespace beast::json;

// NOTE: rtsm::Parser consumes ',' as a separator token without validating
// structural context. Trailing commas are always accepted regardless of
// ParseOptions (allow_trailing_commas not enforced).

TEST(TrailingCommas, ArrayTrailingCommaAccepted) {
  EXPECT_NO_THROW(parse("[1, 2, 3, ]"));
  EXPECT_NO_THROW(parse("[\"a\", ]"));
  EXPECT_NO_THROW(parse("[[], ]"));
}

TEST(TrailingCommas, ObjectTrailingCommaAccepted) {
  EXPECT_NO_THROW(parse("{\"a\": 1, }"));
  EXPECT_NO_THROW(parse("{\"a\": {\"b\": 1, }, }"));
}

// lazy::parse_reuse also accepts trailing commas
TEST(TrailingCommas, LazyParserAcceptsTrailingComma) {
  std::string arr = "[1, 2, ]";
  lazy::DocumentView doc;
  EXPECT_NO_THROW(lazy::parse_reuse(doc, arr));

  std::string obj = "{\"k\": 1, }";
  EXPECT_NO_THROW(lazy::parse_reuse(doc, obj));
}

// Round-trip: trailing comma is preserved by dump()
TEST(TrailingCommas, RoundTripPreservesStructure) {
  // lazy parser accepts "[1,2,]" and dump() re-serializes from tape
  // Note: dump() emits compact JSON without trailing commas
  lazy::DocumentView doc;
  auto v = lazy::parse_reuse(doc, "[1,2,]");
  std::string out = v.dump();
  // dump() does not emit trailing commas; re-serializes from tape nodes
  EXPECT_EQ(out, "[1,2]");
}
