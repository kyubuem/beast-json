#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string_view>

using namespace beast::json;

class ControlChar : public ::testing::Test {
protected:
  bool should_fail(std::string_view json) {
    tape::Document doc;
    tape::Parser p(doc, json.data(), json.size());
    return !p.parse();
  }
};

TEST_F(ControlChar, ValidStrings) {
  EXPECT_FALSE(should_fail(R"({"a": "valid"})"));
}

TEST_F(ControlChar, InvalidControlChars) {
  // Invalid Control Chars (Literal 0x00-0x1F)

  std::string bad_newline = "{\"key\": \"line1\nline2\"}";
  EXPECT_TRUE(should_fail(bad_newline)) << "Accepted literal newline";

  std::string bad_tab = "{\"key\": \"tab\tchar\"}";
  EXPECT_TRUE(should_fail(bad_tab)) << "Accepted literal tab";

  std::string bad_null = std::string("{\"key\": \"null") + '\0' + "\"}";
  EXPECT_TRUE(should_fail(bad_null)) << "Accepted literal null";
}
