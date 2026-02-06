#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

TEST(Serializer, BasicTypes) {
  std::string json = "[null,true,false,123,-456,3.14,\"hello\"]";
  beast::json::tape::Document doc;
  beast::json::tape::Parser parser(doc, json.data(), json.size());
  parser.parse();

  std::string out;
  beast::json::tape::TapeSerializerExtreme serializer(doc, out);
  serializer.serialize();

  EXPECT_EQ(out, json);
}

TEST(Serializer, Nested) {
  std::string json = "{\"a\":[1,2],\"b\":{\"c\":3}}";
  beast::json::tape::Document doc;
  beast::json::tape::Parser parser(doc, json.data(), json.size());
  parser.parse();

  std::string out;
  beast::json::tape::TapeSerializerExtreme serializer(doc, out);
  serializer.serialize();

  EXPECT_EQ(out, json);
}

TEST(Serializer, DeepNesting) {
  std::string json;
  int depth = 100; // Exceeds 64
  for (int i = 0; i < depth; i++)
    json += "{\"a\":";
  json += "1";
  for (int i = 0; i < depth; i++)
    json += "}";

  beast::json::tape::Document doc;
  beast::json::tape::Parser parser(doc, json.data(), json.size());
  parser.parse();

  std::string out;
  beast::json::tape::TapeSerializerExtreme serializer(doc, out);
  serializer.serialize();

  EXPECT_EQ(out, json);
}
