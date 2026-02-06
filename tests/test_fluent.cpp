#include <beast_json/beast_json.hpp>
#include <cmath>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

TEST(Fluent, BasicAccess) {
  tape::Document doc;
  std::string json =
      R"({"id": 42, "name": "Fluent", "enabled": true, "score": 3.14})";
  tape::Parser p(doc, json.data(), json.size());
  p.parse();

  // 1. Root Access
  tape::TapeView root = doc; // Implicit conversion via constructor

  // 2. Implicit Conversions
  int id = root["id"];
  std::string name = root["name"];
  bool enabled = root["enabled"];
  double score = root["score"];

  EXPECT_EQ(id, 42);
  EXPECT_EQ(name, "Fluent");
  EXPECT_EQ(enabled, true);
  EXPECT_NEAR(score, 3.14, 0.001);

  // 3. value_or
  int missing = root["missing"].value_or(100);
  EXPECT_EQ(missing, 100);

  // Type mismatch fallback
  int bad_type = root["name"].value_or(999);
  // Expect 999 because "name" is string, we asked for int
  EXPECT_EQ(bad_type, 999);
}

TEST(Fluent, ArrayAccess) {
  // 4. Array Access
  std::string json_arr = "[10, 20, 30]";
  tape::Document doc2;
  tape::Parser p2(doc2, json_arr.data(), json_arr.size());
  p2.parse();
  tape::TapeView root2 = doc2;

  int v0 = root2[0];
  int v1 = root2[1];
  int v2 = root2[2];

  EXPECT_EQ(v0, 10);
  EXPECT_EQ(v1, 20);
  EXPECT_EQ(v2, 30);
}
