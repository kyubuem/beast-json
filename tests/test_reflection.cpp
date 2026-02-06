#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>
#include <vector>

// 1. Define Structs
struct User {
  int id;
  std::string name;
  double score;
};

struct Group {
  std::string group_name;
  std::vector<User> members;
};

// 2. Register Reflection
BEAST_DEFINE_JSON(User, id, name, score)
BEAST_DEFINE_JSON(Group, group_name, members)

TEST(Reflection, SimpleSerialization) {
  User u{101, "Beast", 99.5};
  std::string json = beast::json::json_of(u);

  // Check key presence (order is deterministic)
  EXPECT_NE(json.find("\"id\":101"), std::string::npos);
  EXPECT_NE(json.find("\"name\":\"Beast\""), std::string::npos);
  EXPECT_NE(json.find("\"score\":99.5"), std::string::npos);
}

TEST(Reflection, NestedSerialization) {
  Group g;
  g.group_name = "Admins";
  g.members.push_back({1, "Alice", 10.0});
  g.members.push_back({2, "Bob", 20.0});

  std::string json = beast::json::json_of(g);

  EXPECT_NE(json.find("\"group_name\":\"Admins\""), std::string::npos);
  EXPECT_NE(json.find("\"members\":[{"), std::string::npos);
  EXPECT_NE(json.find("\"name\":\"Alice\""), std::string::npos);
}

TEST(Reflection, Deserialization) {
  // 1. Struct Parsing
  std::string json_u = R"({"id":200, "name":"Parsed", "score":1.1})";
  User u;
  beast::json::Error err = beast::json::parse_into(u, json_u);
  ASSERT_EQ(err, beast::json::Error::Ok);

  EXPECT_EQ(u.id, 200);
  EXPECT_EQ(u.name, "Parsed");
  EXPECT_GT(u.score, 1.0);
  EXPECT_LT(u.score, 1.2);

  // 2. Nested Parsing
  std::string json_g =
      R"({"group_name":"G1", "members":[{"id":300, "name":"M1", "score":0.0}]})";
  Group g;
  err = beast::json::parse_into(g, json_g);
  ASSERT_EQ(err, beast::json::Error::Ok);

  EXPECT_EQ(g.group_name, "G1");
  ASSERT_EQ(g.members.size(), 1);
  EXPECT_EQ(g.members[0].id, 300);

  // 3. Validation Failure (Type Mismatch)
  std::string bad_int = R"({"id":"NOT_INT", "name":"Bad", "score":0.0})";
  User u_bad;
  err = beast::json::parse_into(u_bad, bad_int);
  EXPECT_EQ(err, beast::json::Error::TypeMismatch);
}
