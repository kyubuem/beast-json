#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>

using namespace beast::json;

TEST(JsonPointer, RFC6901) {
  std::string json = R"({
        "foo": ["bar", "baz"],
        "a/b": 1,
        "c%d": 2,
        "e^f": 3,
        "g|h": 4,
        "i\\j": 5,
        "k\"l": 6,
        " ": 7,
        "m~n": 8
    })";

  tape::Document doc;
  tape::Parser p(doc, json.data(), json.size());
  ASSERT_TRUE(p.parse()) << "Parse Failed";
  tape::TapeView root(doc, 0);

  // RFC 6901 Examples
  EXPECT_TRUE(root.at("").is_object());
  EXPECT_TRUE(root.at("/foo").is_array());
  EXPECT_EQ(root.at("/foo/0").get_string(), "bar");
  EXPECT_EQ(root.at("/foo/1").get_string(), "baz");

  EXPECT_EQ(root.at("/a~1b").get_int64(), 1);
  EXPECT_EQ(root.at("/c%d").get_int64(), 2);
  EXPECT_EQ(root.at("/e^f").get_int64(), 3);
  EXPECT_EQ(root.at("/g|h").get_int64(), 4);
  EXPECT_EQ(root.at("/i\\j").get_int64(), 5);
  EXPECT_EQ(root.at("/k\"l").get_int64(), 6);
  EXPECT_EQ(root.at("/ ").get_int64(), 7);
  EXPECT_EQ(root.at("/m~0n").get_int64(), 8);
}
