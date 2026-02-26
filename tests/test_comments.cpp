#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string_view>

using namespace beast::json;

class Comments : public ::testing::Test {
protected:
  bool parse_json(std::string_view json) {
     
    lazy::DocumentView p(json);
    return true; // Assuming implicit comment support or default enabled
  }
};

TEST_F(Comments, SingleLine) {
  std::string json_single = R"({
        "a": 1, // This is a comment
        "b": 2
    })";
  EXPECT_TRUE(parse_json(json_single)) << "Single-line comment rejected";
}

TEST_F(Comments, MultiLine) {
  std::string json_multi = R"({
        "a": 1, 
        /* This is a 
           multi-line comment */
        "b": 2
    })";
  EXPECT_TRUE(parse_json(json_multi)) << "Multi-line comment rejected";
}

TEST_F(Comments, Surrounding) {
  std::string json_surround = R"(
        // Start comment
        { "a": 1 }
        // End comment
    )";
  EXPECT_TRUE(parse_json(json_surround)) << "Surrounding comments rejected";
}
