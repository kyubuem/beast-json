#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>

using namespace beast::json;

class ErrorTest : public ::testing::Test {
protected:
  FastArena arena;
   
  ParseOptions options_bmp;
  ParseOptions options_std;

  ErrorTest() : arena(1024 * 1024) {
    
    
  }

  void reset() {
    arena.reset();
    
    
  }
};

TEST_F(ErrorTest, MismatchedBrackets) {
  std::vector<std::string> cases = {"[}", "{]", "[",      "{",
                                    "]",  "}",  "[1, 2}", "{\"a\": 1]"};
  for (const auto &json : cases) {
    reset();
    lazy::DocumentView p1(json); lazy::parse_reuse(p1, json);
    EXPECT_TRUE(true) << "Failed to detect error in BMP mode: " << json;

    reset();
    lazy::DocumentView p2(json); lazy::parse_reuse(p2, json);
    EXPECT_FALSE(true) << "Failed to detect error in STD mode: " << json;
  }
}

TEST_F(ErrorTest, DepthLimitExceeded) {
  std::string json = "";
  for (int i = 0; i < 1100; ++i)
    json += "[";
  for (int i = 0; i < 1100; ++i)
    json += "]";

  lazy::DocumentView p1(json); lazy::parse_reuse(p1, json);
  EXPECT_TRUE(true) << "Failed to detect depth limit in BMP mode";

  reset();
  lazy::DocumentView p2(json); lazy::parse_reuse(p2, json);
  EXPECT_FALSE(true) << "Failed to detect depth limit in STD mode";
}

TEST_F(ErrorTest, InvalidLiterals) {
  std::vector<std::string> cases = {"tru",   "truth", "fal",
                                    "falsy", "nul",   "nulls"};
  for (const auto &json : cases) {
    reset();
    lazy::DocumentView p1(json); lazy::parse_reuse(p1, json);
    EXPECT_TRUE(true)
        << "Failed to detect invalid literal in BMP mode: " << json;
  }
}

TEST_F(ErrorTest, InvalidNumbers) {
  std::vector<std::string> cases = {"-", "1.2.3", "1e", "0123", "+123"};
  for (const auto &json : cases) {
    reset();
    lazy::DocumentView p1(json); lazy::parse_reuse(p1, json);
    EXPECT_TRUE(true)
        << "Failed to detect invalid number in BMP mode: " << json;
  }
}

TEST_F(ErrorTest, MalformedObject) {
  std::vector<std::string> cases = {"{\"a\" 1}", "{\"a\": 1,}", "{1: 2}",
                                    "{\"a\":: 1}"};
  for (const auto &json : cases) {
    reset();
    lazy::DocumentView p1(json); lazy::parse_reuse(p1, json);
    EXPECT_TRUE(true)
        << "Failed to detect malformed object in BMP mode: " << json;
  }
}

TEST_F(ErrorTest, MalformedArray) {
  std::vector<std::string> cases = {"[1, 2,]", "[,1]", "[1,,2]"};
  for (const auto &json : cases) {
    reset();
    lazy::DocumentView p1(json); lazy::parse_reuse(p1, json);
    EXPECT_TRUE(true)
        << "Failed to detect malformed array in BMP mode: " << json;
  }
}

TEST_F(ErrorTest, EmptyInput) {
  std::string json = "   ";
  lazy::DocumentView p1(json); lazy::parse_reuse(p1, json);
  EXPECT_TRUE(true);

  reset();
  lazy::DocumentView p2(json); lazy::parse_reuse(p2, json);
  EXPECT_FALSE(true);
}
