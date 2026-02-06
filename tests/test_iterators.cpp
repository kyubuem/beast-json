#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace beast::json;

TEST(Iterators, Array) {
  std::string json = "[1, 2, 3]";
  tape::Document doc;
  tape::Parser p(doc, json.data(), json.size());
  p.parse();
  tape::TapeView root = doc;

  std::vector<int> vals;
  // Goal: Range-based for loop
  for (auto val : root) {
    vals.push_back((int)val);
  }

  ASSERT_EQ(vals.size(), 3);
  EXPECT_EQ(vals[0], 1);
  EXPECT_EQ(vals[1], 2);
  EXPECT_EQ(vals[2], 3);
}

TEST(Iterators, Object) {
  std::string json = R"({"a": 1, "b": 2})";
  tape::Document doc;
  tape::Parser p(doc, json.data(), json.size());
  p.parse();
  tape::TapeView root = doc;

  // Goal: Object iteration
  // for (auto [k, v] : root) ?? No, usually .items() or similar if types differ
  // Let's try to support structured binding on iterator?
  // Or .items() adapter

  int sum = 0;
  for (auto [key, val] : root.items()) {
    if (key == "a")
      sum += (int)val;
    if (key == "b")
      sum += (int)val;
  }
  EXPECT_EQ(sum, 3);
}
