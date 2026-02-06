#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

TEST(DuplicateKeys, LastWinsDefault) {
  std::string json = R"({"key": 1, "key": 2, "other": 3, "key": 99})";

  Value v = parse(json); // Uses default tape behavior (last wins) implicitly if
                         // parsing to Value uses Tape

  // Ideally we test Tape directly as original test did, or rely on Value
  // wrapper
  EXPECT_EQ(v["key"].as_int(), 99);
  EXPECT_EQ(v["other"].as_int(), 3);
}
