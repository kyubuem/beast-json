#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace beast::json;

class Utf8Validation : public ::testing::TestWithParam<
                           std::tuple<std::string, std::string, bool>> {
protected:
  bool check_utf8(std::string_view json) {
    tape::Document doc;
    tape::Parser p(doc, json.data(), json.size());
    auto res = p.parse();
    return res.error == beast::json::Error::Ok;
  }
};

TEST_P(Utf8Validation, Check) {
  auto [name, json, should_pass] = GetParam();
  EXPECT_EQ(check_utf8(json), should_pass) << "Failed case: " << name;
}

INSTANTIATE_TEST_SUITE_P(
    AllCases, Utf8Validation,
    ::testing::Values(
        std::make_tuple("ASCII", R"({"key": "value"})", true),
        std::make_tuple("Valid 2-byte", "{\"key\": \"\xC2\xA2\"}", true),
        std::make_tuple("Valid 3-byte", "{\"key\": \"\xE2\x82\xAC\"}", true),
        std::make_tuple("Valid 4-byte", "{\"key\": \"\xF0\x9D\x84\x9E\"}",
                        true),

        // Invalid Start
        std::make_tuple("Invalid Start 0x80", "{\"key\": \"\x80\"}", false),
        std::make_tuple("Invalid Start 0xC0", "{\"key\": \"\xC0\xAF\"}", false),
        std::make_tuple("Invalid Start 0xFF", "{\"key\": \"\xFF\"}", false),

        // Overlong
        std::make_tuple("Overlong 2-byte", "{\"key\": \"\xC0\xAF\"}", false),
        std::make_tuple("Overlong 3-byte", "{\"key\": \"\xE0\x80\xAF\"}",
                        false),

        // Missing Continuation
        std::make_tuple("Missing Continuation", "{\"key\": \"\xE2\x82\"}",
                        false),
        std::make_tuple("Bad Continuation", "{\"key\": \"\xE2\x02\xAC\"}",
                        false),

        // Raw Surrogate (High)
        std::make_tuple("Raw Surrogate High", "{\"key\": \"\xED\xA0\x80\"}",
                        false),

        // Valid Mixed
        std::make_tuple("Mixed", "{\"key\": \"Hello \xF0\x9F\x8C\x8D World\"}",
                        true)),
    [](const testing::TestParamInfo<Utf8Validation::ParamType> &info) {
      std::string name = std::get<0>(info.param);
      // Replace spaces with underscores for GTest name compatibility if needed,
      // but string parameter is just for display invocation usually.
      // Actually GTest names must be alphanumeric. We'll rely on index or
      // sanitize. Simple sanitization:
      std::replace(name.begin(), name.end(), ' ', '_');
      std::replace(name.begin(), name.end(), '-', '_');
      std::replace(name.begin(), name.end(), '(', '_');
      std::replace(name.begin(), name.end(), ')', '_');
      return name;
    });
