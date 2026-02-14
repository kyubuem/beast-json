#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>

using namespace beast::json;

class ErrorTest : public ::testing::Test {
protected:
  FastArena arena;
  tape::Document doc;
  tape::ParseOptions options_bmp;
  tape::ParseOptions options_std;

  ErrorTest() : arena(1024 * 1024), doc(&arena) {
    options_bmp.use_bitmap = true;
    options_std.use_bitmap = false;
  }

  void reset() {
    arena.reset();
    doc.tape.clear();
    doc.string_buffer.clear();
  }
};

TEST_F(ErrorTest, MismatchedBrackets) {
  std::vector<std::string> cases = {"[}", "{]", "[",      "{",
                                    "]",  "}",  "[1, 2}", "{\"a\": 1]"};
  for (const auto &json : cases) {
    reset();
    tape::Parser p1(doc, json.data(), json.size(), options_bmp);
    EXPECT_FALSE(p1.parse()) << "Failed to detect error in BMP mode: " << json;

    reset();
    tape::Parser p2(doc, json.data(), json.size(), options_std);
    EXPECT_FALSE(p2.parse()) << "Failed to detect error in STD mode: " << json;
  }
}

TEST_F(ErrorTest, DepthLimitExceeded) {
  std::string json = "";
  for (int i = 0; i < 1100; ++i)
    json += "[";
  for (int i = 0; i < 1100; ++i)
    json += "]";

  tape::Parser p1(doc, json.data(), json.size(), options_bmp);
  EXPECT_FALSE(p1.parse()) << "Failed to detect depth limit in BMP mode";

  reset();
  tape::Parser p2(doc, json.data(), json.size(), options_std);
  EXPECT_FALSE(p2.parse()) << "Failed to detect depth limit in STD mode";
}

TEST_F(ErrorTest, InvalidLiterals) {
  std::vector<std::string> cases = {"tru",   "truth", "fal",
                                    "falsy", "nul",   "nulls"};
  for (const auto &json : cases) {
    reset();
    tape::Parser p1(doc, json.data(), json.size(), options_bmp);
    EXPECT_FALSE(p1.parse())
        << "Failed to detect invalid literal in BMP mode: " << json;
  }
}

TEST_F(ErrorTest, InvalidNumbers) {
  std::vector<std::string> cases = {"-", "1.2.3", "1e", "0123", "+123"};
  for (const auto &json : cases) {
    reset();
    tape::Parser p1(doc, json.data(), json.size(), options_bmp);
    EXPECT_FALSE(p1.parse())
        << "Failed to detect invalid number in BMP mode: " << json;
  }
}

TEST_F(ErrorTest, MalformedObject) {
  std::vector<std::string> cases = {"{\"a\" 1}", "{\"a\": 1,}", "{1: 2}",
                                    "{\"a\":: 1}"};
  for (const auto &json : cases) {
    reset();
    tape::Parser p1(doc, json.data(), json.size(), options_bmp);
    EXPECT_FALSE(p1.parse())
        << "Failed to detect malformed object in BMP mode: " << json;
  }
}

TEST_F(ErrorTest, MalformedArray) {
  std::vector<std::string> cases = {"[1, 2,]", "[,1]", "[1,,2]"};
  for (const auto &json : cases) {
    reset();
    tape::Parser p1(doc, json.data(), json.size(), options_bmp);
    EXPECT_FALSE(p1.parse())
        << "Failed to detect malformed array in BMP mode: " << json;
  }
}

TEST_F(ErrorTest, EmptyInput) {
  std::string json = "   ";
  tape::Parser p1(doc, json.data(), json.size(), options_bmp);
  EXPECT_FALSE(p1.parse());

  reset();
  tape::Parser p2(doc, json.data(), json.size(), options_std);
  EXPECT_FALSE(p2.parse());
}
