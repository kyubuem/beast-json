#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>

using namespace beast::json;

class FlatLoopTest : public ::testing::Test {
protected:
  FastArena arena;
  tape::Document doc;
  tape::ParseOptions options;

  FlatLoopTest() : arena(1024 * 1024), doc(&arena) {
    options.use_bitmap = true;
  }

  void reset() {
    arena.reset();
    doc.tape.clear();
    doc.string_buffer.clear();
  }
};

TEST_F(FlatLoopTest, SimpleAtoms) {
  std::string json = "true";
  tape::Parser p(doc, json.data(), json.size(), options);
  EXPECT_TRUE(p.parse());
  EXPECT_EQ(doc.tape.size(), 1); // Atom
  EXPECT_EQ(doc.tape[0] >> 56, (uint64_t)tape::Type::True);

  reset();
  json = "false";
  tape::Parser p2(doc, json.data(), json.size(), options);
  EXPECT_TRUE(p2.parse());
  EXPECT_EQ(doc.tape[0] >> 56, (uint64_t)tape::Type::False);

  reset();
  json = "null";
  tape::Parser p3(doc, json.data(), json.size(), options);
  EXPECT_TRUE(p3.parse());
  EXPECT_EQ(doc.tape[0] >> 56, (uint64_t)tape::Type::Null);
}

TEST_F(FlatLoopTest, Numbers) {
  std::string json = "123";
  tape::Parser p(doc, json.data(), json.size(), options);
  EXPECT_TRUE(p.parse());
  EXPECT_EQ(doc.tape[0] >> 56, (uint64_t)tape::Type::Int64);

  reset();
  json = "-456";
  tape::Parser p2(doc, json.data(), json.size(), options);
  EXPECT_TRUE(p2.parse());
  EXPECT_EQ(doc.tape[0] >> 56, (uint64_t)tape::Type::Int64);

  reset();
  json = "123.456";
  tape::Parser p3(doc, json.data(), json.size(), options);
  EXPECT_TRUE(p3.parse());
  EXPECT_EQ(doc.tape[0] >> 56, (uint64_t)tape::Type::Double);
}

TEST_F(FlatLoopTest, SimpleObject) {
  std::string json = "{\"a\": 1, \"b\": true}";
  tape::Parser p(doc, json.data(), json.size(), options);
  EXPECT_TRUE(p.parse());
  // Tape format: [Object Start, Key "a", Val 1, Key "b", Val true, Object End]
  EXPECT_EQ(doc.tape.size(), 6);
  EXPECT_EQ(doc.tape[0] >> 56, (uint64_t)tape::Type::Object);
  EXPECT_EQ(doc.tape[1] >> 56, (uint64_t)tape::Type::String);
}

TEST_F(FlatLoopTest, SimpleArray) {
  std::string json = "[1, 2, 3]";
  tape::Parser p(doc, json.data(), json.size(), options);
  EXPECT_TRUE(p.parse());
  // Tape format: [Array Start, 1, 2, 3, Array End]
  EXPECT_EQ(doc.tape.size(), 5);
  EXPECT_EQ(doc.tape[0] >> 56, (uint64_t)tape::Type::Array);
}

TEST_F(FlatLoopTest, NestedStructures) {
  std::string json = "{\"list\": [1, {\"inner\": null}, 3]}";
  tape::Parser p(doc, json.data(), json.size(), options);
  EXPECT_TRUE(p.parse());
  EXPECT_EQ(doc.tape[0] >> 56, (uint64_t)tape::Type::Object);
}

TEST_F(FlatLoopTest, DeepNesting) {
  std::string json = "";
  for (int i = 0; i < 50; ++i)
    json += "[";
  for (int i = 0; i < 50; ++i)
    json += "]";
  tape::Parser p(doc, json.data(), json.size(), options);
  EXPECT_TRUE(p.parse());
}

TEST_F(FlatLoopTest, WhitespaceAndNewlines) {
  std::string json = "  \n {\t\"key\"  : \r 123 \n } \f ";
  tape::Parser p(doc, json.data(), json.size(), options);
  EXPECT_TRUE(p.parse());
}
