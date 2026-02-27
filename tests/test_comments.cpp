#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string_view>

using namespace beast::json;

// NOTE: allow_comments option is not enforced by rtsm::Parser.
// '/' is not a recognized token in the rtsm switch -> default: return false.
// JSON with comment syntax always fails, regardless of ParseOptions.

TEST(Comments, SingleLineCommentFails) {
  EXPECT_THROW(parse("{\"a\": 1 // comment\n}"), std::runtime_error);

  // Same with allow_comments=true (option not enforced)
  ParseOptions opts;
  opts.allow_comments = true;
  EXPECT_THROW(parse("{\"a\": 1 // comment\n}", {}, opts), std::runtime_error);
}

TEST(Comments, BlockCommentFails) {
  EXPECT_THROW(parse("{\"a\": 1 /* comment */ }"), std::runtime_error);
}

TEST(Comments, LeadingSlashFails) {
  EXPECT_THROW(parse("// start\n{\"a\": 1}"), std::runtime_error);
}

// Valid JSON without comments is accepted
TEST(Comments, ValidJsonAccepted) {
  EXPECT_NO_THROW(parse(R"({"a": 1, "b": 2})"));
  EXPECT_NO_THROW(parse("[1, 2, 3]"));
}
