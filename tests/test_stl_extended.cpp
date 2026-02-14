#include <array>
#include <beast_json/beast_json.hpp>
#include <filesystem>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

// Define a struct that uses extended STL types to verify macro integration
struct ExtendedStruct {
  std::pair<int, std::string> p;
  std::tuple<int, double, std::string> t;
  std::array<int, 3> a;
  std::variant<int, std::string> v;
  std::filesystem::path path;

  bool operator==(const ExtendedStruct &other) const {
    return p == other.p && t == other.t && a == other.a && v == other.v &&
           path == other.path;
  }
};

BEAST_DEFINE_JSON(ExtendedStruct, p, t, a, v, path)

TEST(StlExtended, PairRoundTrip) {
  std::pair<int, std::string> p{42, "meaning"};
  std::string json = beast::json::json_of(p);
  // Expected: [42,"meaning"]

  std::pair<int, std::string> p2;
  beast::json::Error err = beast::json::parse_into(p2, json);
  ASSERT_EQ(err, beast::json::Error::Ok);
  EXPECT_EQ(p, p2);
}

TEST(StlExtended, TupleRoundTrip) {
  std::tuple<int, double, std::string> t{1, 3.14, "pi"};
  std::string json = beast::json::json_of(t);

  std::tuple<int, double, std::string> t2;
  beast::json::Error err = beast::json::parse_into(t2, json);
  ASSERT_EQ(err, beast::json::Error::Ok);
  EXPECT_EQ(t, t2);
}

TEST(StlExtended, ArrayRoundTrip) {
  std::array<int, 3> a{10, 20, 30};
  std::string json = beast::json::json_of(a);

  std::array<int, 3> a2;
  beast::json::Error err = beast::json::parse_into(a2, json);
  ASSERT_EQ(err, beast::json::Error::Ok);
  EXPECT_EQ(a, a2);
}

TEST(StlExtended, VariantRoundTrip) {
  // Test int alternative
  std::variant<int, double, std::string> v1 = 123;
  std::string json1 = beast::json::json_of(v1);
  std::variant<int, double, std::string> v1_out;
  ASSERT_EQ(beast::json::parse_into(v1_out, json1), beast::json::Error::Ok);
  EXPECT_EQ(v1, v1_out);
  EXPECT_TRUE(std::holds_alternative<int>(v1_out));

  // Test string alternative
  std::variant<int, double, std::string> v2 = "hello";
  std::string json2 = beast::json::json_of(v2);
  std::variant<int, double, std::string> v2_out;
  ASSERT_EQ(beast::json::parse_into(v2_out, json2), beast::json::Error::Ok);
  EXPECT_EQ(v2, v2_out);
  EXPECT_TRUE(std::holds_alternative<std::string>(v2_out));

  // Test double alternative
  std::variant<int, double, std::string> v3 = 3.14159;
  std::string json3 = beast::json::json_of(v3);
  std::variant<int, double, std::string> v3_out;
  ASSERT_EQ(beast::json::parse_into(v3_out, json3), beast::json::Error::Ok);
  // Verify value approx equality since variants compare exactly
  ASSERT_TRUE(std::holds_alternative<double>(v3_out));
  EXPECT_NEAR(std::get<double>(v3), std::get<double>(v3_out), 0.0001);
}

TEST(StlExtended, FilesystemPathRoundTrip) {
  std::filesystem::path p = "/usr/local/bin";
  std::string json = beast::json::json_of(p);

  std::filesystem::path p2;
  beast::json::Error err = beast::json::parse_into(p2, json);
  ASSERT_EQ(err, beast::json::Error::Ok);
  EXPECT_EQ(p, p2);
}

TEST(StlExtended, NestedContainers) {
  std::vector<std::variant<int, std::string>> vec;
  vec.push_back(10);
  vec.push_back("test");
  vec.push_back(20);

  std::string json = beast::json::json_of(vec);

  std::vector<std::variant<int, std::string>> vec2;
  beast::json::Error err = beast::json::parse_into(vec2, json);
  ASSERT_EQ(err, beast::json::Error::Ok);
  EXPECT_EQ(vec, vec2);
}

TEST(StlExtended, ExtendedStructRoundTrip) {
  ExtendedStruct s;
  s.p = {99, "balloons"};
  s.t = {7, 1.414, "sqrt2"};
  s.a = {1, 2, 3};
  s.v = "string_variant";
  s.path = "tmp/test.txt";

  std::string json = beast::json::json_of(s);
  // std::cout << "ExtendedStruct JSON: " << json << std::endl;

  ExtendedStruct s2;
  beast::json::Error err = beast::json::parse_into(s2, json);
  ASSERT_EQ(err, beast::json::Error::Ok);
  EXPECT_EQ(s, s2);
}
