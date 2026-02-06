#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

TEST(RelaxedParsing, StrictDuplicateKeys) {
  std::string json = R"({"a": 1, "a": 2})";

  // Default: Allowed (Last wins)
  EXPECT_NO_THROW({
    Value v = parse(json);
    EXPECT_EQ(v["a"].as_int(), 2);
  });

  // Strict: Error
  ParseOptions opts;
  opts.allow_duplicate_keys = false;
  EXPECT_THROW(
      { parse(json, {}, opts); }, ParseError)
      << "Should throw ParseError on duplicate keys in strict mode";
}

TEST(RelaxedParsing, SingleQuotes) {
  std::string json = "{'a': 'b'}";

  // Default: Error
  EXPECT_THROW(parse(json), ParseError);

  // Relaxed: Allowed
  ParseOptions opts;
  opts.allow_single_quotes = true;
  EXPECT_NO_THROW({
    Value v = parse(json, {}, opts);
    EXPECT_EQ(v["a"].as_string(), "b");
  });
}

TEST(RelaxedParsing, UnquotedKeys) {
  std::string json = "{a: 1, $b_: 2}";

  // Default: Error
  EXPECT_THROW(parse(json), ParseError);

  // Relaxed: Allowed
  ParseOptions opts;
  opts.allow_unquoted_keys = true;
  EXPECT_NO_THROW({
    Value v = parse(json, {}, opts);
    EXPECT_EQ(v["a"].as_int(), 1);
    EXPECT_EQ(v["$b_"].as_int(), 2);
  });
}

TEST(RelaxedParsing, MixedRelaxed) {
  std::string json = "{key: 'value', 'num': 123}";
  ParseOptions opts;
  opts.allow_single_quotes = true;
  opts.allow_unquoted_keys = true;

  EXPECT_NO_THROW({
    Value v = parse(json, {}, opts);
    EXPECT_EQ(v["key"].as_string(), "value");
    EXPECT_EQ(v["num"].as_int(), 123);
  });
}
