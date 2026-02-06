#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

TEST(StringStructure, NotParsedAsObject) {
  // Tests that { } [ ] inside strings are NOT parsed as objects/arrays
  std::string json = R"({
        "config": "{ \"enabled\": true, \"flags\": [1,2,3] }",
        "description": "This is a [bracket] and {brace} test"
    })";

  tape::Document doc;
  tape::Parser p(doc, json.data(), json.size());
  auto res = p.parse();

  ASSERT_TRUE(res) << "Parse Failed: " << res.message;

  tape::TapeView root(doc, 0);

  // 1. Verify "config" is a STRING, not an Object
  tape::TapeView config = root["config"];
  EXPECT_TRUE(config.is_string());
  EXPECT_FALSE(config.is_object());

  std::string_view val = config.get_string();
  std::string expected = "{ \"enabled\": true, \"flags\": [1,2,3] }";
  EXPECT_EQ(val, expected);

  // 2. Verify description
  tape::TapeView desc = root["description"];
  EXPECT_EQ(desc.get_string(), "This is a [bracket] and {brace} test");
}
