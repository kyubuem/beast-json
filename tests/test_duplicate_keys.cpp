#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

// NOTE: rtsm::Parser does not enforce allow_duplicate_keys.
// Duplicate keys are always accepted regardless of ParseOptions.

TEST(DuplicateKeys, AlwaysAcceptedByRtsm) {
  EXPECT_NO_THROW(parse(R"({"key": 1, "key": 2})"));
  EXPECT_NO_THROW(parse(R"({"a": 1, "b": 2, "a": 3, "a": 99})"));

  // With strict option (not enforced in rtsm)
  ParseOptions strict;
  strict.allow_duplicate_keys = false;
  EXPECT_NO_THROW(parse(R"({"key": 1, "key": 2})", {}, strict));
}

// lazy parser also accepts duplicate keys
TEST(DuplicateKeys, LazyRoundTrip) {
  std::string json = R"({"a":1,"a":2,"b":3})";
  lazy::DocumentView doc;
  auto v = lazy::parse_reuse(doc, json);
  // dump() preserves all occurrences from the tape
  EXPECT_EQ(v.dump(), json);
}

// Multiple duplicates: all entries preserved in tape
TEST(DuplicateKeys, MultipleDuplicatesPreserved) {
  std::string json = R"({"x":1,"x":2,"x":3})";
  lazy::DocumentView doc;
  auto v = lazy::parse_reuse(doc, json);
  EXPECT_EQ(v.dump(), json);
}
