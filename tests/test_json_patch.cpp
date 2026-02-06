#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

TEST(JsonPatch, AddObjectMember) {
  Value doc = Value::object();
  doc["foo"] = "bar";

  Value patch = Value::array();
  Value op = Value::object();
  op["op"] = "add";
  op["path"] = "/baz";
  op["value"] = "qux";
  patch.push_back(std::move(op));

  apply_patch(doc, patch);

  EXPECT_EQ(doc["baz"].as_string(), "qux");
}

TEST(JsonPatch, AddArrayElement) {
  Value arr_doc = Value::array();
  arr_doc.push_back("foo");

  Value patch2 = Value::array();
  Value op2 = Value::object();
  op2["op"] = "add";
  op2["path"] = "/0";
  op2["value"] = "bar";
  patch2.push_back(std::move(op2));

  apply_patch(arr_doc, patch2);

  EXPECT_EQ(arr_doc[0].as_string(), "bar");
  EXPECT_EQ(arr_doc[1].as_string(), "foo");
}

TEST(JsonPatch, RemoveObjectMember) {
  Value doc = Value::object();
  doc["foo"] = "bar";
  doc["baz"] = "qux";

  Value patch = Value::array();
  Value op = Value::object();
  op["op"] = "remove";
  op["path"] = "/baz";
  patch.push_back(std::move(op));

  apply_patch(doc, patch);

  EXPECT_FALSE(doc.as_object().contains("baz"));
}

TEST(JsonPatch, ReplaceObjectMember) {
  Value doc = Value::object();
  doc["foo"] = "bar";

  Value patch = Value::array();
  Value op = Value::object();
  op["op"] = "replace";
  op["path"] = "/foo";
  op["value"] = "baz";
  patch.push_back(std::move(op));

  apply_patch(doc, patch);

  EXPECT_EQ(doc["foo"].as_string(), "baz");
}

TEST(JsonPatch, MoveObjectMember) {
  Value doc = Value::object();
  Value foo = Value::object();
  foo["bar"] = "baz";
  foo["waldo"] = "fred";
  doc["foo"] = std::move(foo);
  doc["qux"] = "corge";

  Value patch = Value::array();
  Value op = Value::object();
  op["op"] = "move";
  op["from"] = "/foo/waldo";
  op["path"] = "/qux";
  patch.push_back(std::move(op));

  apply_patch(doc, patch);

  // Result: { "foo": { "bar": "baz" }, "qux": "fred" }
  EXPECT_EQ(doc["qux"].as_string(), "fred");
  EXPECT_FALSE(doc["foo"].as_object().contains("waldo"));
}

TEST(JsonPatch, CopyObjectMember) {
  Value doc = Value::object();
  doc["foo"] = "bar";

  Value patch = Value::array();
  Value op = Value::object();
  op["op"] = "copy";
  op["from"] = "/foo";
  op["path"] = "/baz";
  patch.push_back(std::move(op));

  apply_patch(doc, patch);

  EXPECT_EQ(doc["baz"].as_string(), "bar");
  EXPECT_EQ(doc["foo"].as_string(), "bar");
}

TEST(JsonPatch, TestOp) {
  Value doc = Value::object();
  doc["foo"] = "bar";

  // Test Equality (Pass)
  Value patch = Value::array();
  Value op = Value::object();
  op["op"] = "test";
  op["path"] = "/foo";
  op["value"] = "bar";
  patch.push_back(std::move(op));

  EXPECT_NO_THROW(apply_patch(doc, patch));

  // Test Inequality (Fail)
  Value patch2 = Value::array();
  Value op2 = Value::object();
  op2["op"] = "test";
  op2["path"] = "/foo";
  op2["value"] = "baz";
  patch2.push_back(std::move(op2));

  EXPECT_THROW(apply_patch(doc, patch2), std::exception);
}
