#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

TEST(MergePatch, Simple) {
  // Target: {"a": "b"}
  // Patch: {"a": "c"}
  // Result: {"a": "c"}
  Value target = Value::object();
  target["a"] = "b";

  Value patch = Value::object();
  patch["a"] = "c";

  merge_patch(target, patch);

  EXPECT_EQ(target["a"].as_string(), "c");
}

TEST(MergePatch, RemoveMember) {
  // Target: {"a": "b", "c": "d"}
  // Patch: {"a": null}
  // Result: {"c": "d"}
  Value target = Value::object();
  target["a"] = "b";
  target["c"] = "d";

  Value patch = Value::object();
  patch["a"] = Value::null();

  merge_patch(target, patch);

  EXPECT_FALSE(target.as_object().contains("a"));
  EXPECT_TRUE(target.as_object().contains("c"));
}

TEST(MergePatch, AddNested) {
  // Target: {"a": "b"}
  // Patch: {"c": {"d": "e"}}
  // Result: {"a": "b", "c": {"d": "e"}}
  Value target = Value::object();
  target["a"] = "b";

  Value patch = Value::object();
  Value nested = Value::object();
  nested["d"] = "e";
  patch["c"] = std::move(nested);

  merge_patch(target, patch);

  EXPECT_EQ(target["c"]["d"].as_string(), "e");
}

TEST(MergePatch, RecursiveMerge) {
  // Target: {"a": {"b": "c"}}
  // Patch: {"a": {"b": "d", "e": "f"}}
  // Result: {"a": {"b": "d", "e": "f"}}
  Value target = Value::object();
  Value nested = Value::object();
  nested["b"] = "c";
  target["a"] = std::move(nested);

  Value patch = Value::object();
  Value p_nested = Value::object();
  p_nested["b"] = "d";
  p_nested["e"] = "f";
  patch["a"] = std::move(p_nested);

  merge_patch(target, patch);

  EXPECT_EQ(target["a"]["b"].as_string(), "d");
  EXPECT_EQ(target["a"]["e"].as_string(), "f");
}

TEST(MergePatch, ArrayReplacement) {
  // Target: {"a": [1, 2]}
  // Patch: {"a": [3]}
  // Result: {"a": [3]} -> Arrays are replaced, not merged!
  Value target = Value::object();
  Value arr = Value::array();
  arr.push_back(1);
  arr.push_back(2);
  target["a"] = std::move(arr);

  Value patch = Value::object();
  Value p_arr = Value::array();
  p_arr.push_back(3);
  patch["a"] = std::move(p_arr);

  merge_patch(target, patch);

  ASSERT_TRUE(target["a"].is_array());
  ASSERT_EQ(target["a"].size(), 1);
  EXPECT_EQ(target["a"][0].as_int(), 3);
}
