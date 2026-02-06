#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string_view>

using namespace beast::json;

class StrictNumber : public ::testing::Test {
protected:
  bool should_fail(std::string_view json) {
    tape::Document doc;
    tape::Parser p(doc, json.data(), json.size());
    return !p.parse();
  }
};

TEST_F(StrictNumber, ValidNumbers) {
  EXPECT_FALSE(should_fail("0"));
  EXPECT_FALSE(should_fail("0.123"));
  EXPECT_FALSE(should_fail("-0"));
}

TEST_F(StrictNumber, InvalidNumbers) {
  // Should FAIL (RFC 8259)
  EXPECT_TRUE(should_fail("01")) << "Accepted leading zero 01";
  EXPECT_TRUE(should_fail("007")) << "Accepted 007";
  EXPECT_TRUE(should_fail("1.")) << "Accepted trailing dot 1.";
  EXPECT_TRUE(should_fail(".5")) << "Accepted leading dot .5";
  EXPECT_TRUE(should_fail("1e")) << "Accepted incomplete exponent 1e";
}
