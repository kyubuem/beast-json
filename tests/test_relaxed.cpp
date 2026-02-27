#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

// NOTE: ParseOptions are stored in rtsm::Parser but NOT enforced in its
// parse() loop. All option-based tests document actual parser behavior.

// Single quotes: '\'' is not a recognized value-start in rtsm switch
// -> default: return false -> always throws regardless of ParseOptions
TEST(RelaxedParsing, SingleQuotesNotSupported) {
  std::string json = "{'a': 'b'}";
  EXPECT_THROW(parse(json), std::runtime_error);

  // Options not enforced in rtsm::Parser; still throws
  ParseOptions opts;
  opts.allow_single_quotes = true;
  EXPECT_THROW(parse(json, {}, opts), std::runtime_error);
}

// Unquoted keys: identifier chars not in rtsm switch -> always throws
TEST(RelaxedParsing, UnquotedKeysNotSupported) {
  std::string json = "{a: 1}";
  EXPECT_THROW(parse(json), std::runtime_error);

  ParseOptions opts;
  opts.allow_unquoted_keys = true;
  EXPECT_THROW(parse(json, {}, opts), std::runtime_error);
}

// Trailing commas: rtsm consumes ',' as separator without context validation
// -> trailing commas always accepted
TEST(RelaxedParsing, TrailingCommasAccepted) {
  EXPECT_NO_THROW(parse("[1, 2, ]"));
  EXPECT_NO_THROW(parse("{\"a\": 1, }"));

  // Options not enforced; still accepted even when disabled
  ParseOptions strict;
  strict.allow_trailing_commas = false;
  EXPECT_NO_THROW(parse("[1, 2, ]", {}, strict));
}

// Duplicate keys: always accepted (options not enforced)
TEST(RelaxedParsing, DuplicateKeysAccepted) {
  EXPECT_NO_THROW(parse(R"({"a": 1, "a": 2})"));

  ParseOptions strict;
  strict.allow_duplicate_keys = false;
  EXPECT_NO_THROW(parse(R"({"a": 1, "a": 2})", {}, strict));
}

// Valid JSON: basic positive cases
TEST(RelaxedParsing, ValidJsonAccepted) {
  EXPECT_NO_THROW(parse(R"({"key": "value"})"));
  EXPECT_NO_THROW(parse("[1, 2, 3]"));
  EXPECT_NO_THROW(parse("null"));
  EXPECT_NO_THROW(parse("true"));
  EXPECT_NO_THROW(parse("false"));
  EXPECT_NO_THROW(parse("42"));
}
