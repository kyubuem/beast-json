#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace beast::json;

// Helper: attempt lazy parse, return true on success
static bool lazy_ok(std::string_view j) {
  try {
    lazy::DocumentView doc;
    lazy::parse_reuse(doc, j);
    return true;
  } catch (const std::runtime_error &) {
    return false;
  }
}

// Helper: attempt rtsm parse, return true on success
static bool rtsm_ok(std::string_view j) {
  try {
    beast::json::parse(j);
    return true;
  } catch (const std::runtime_error &) {
    return false;
  }
}

// Unterminated containers: depth != 0 at end → lazy parser returns false
TEST(LazyErrors, UnterminatedContainers) {
  EXPECT_FALSE(lazy_ok("["));
  EXPECT_FALSE(lazy_ok("{"));
  EXPECT_FALSE(lazy_ok("[1, 2"));
  EXPECT_FALSE(lazy_ok("{\"a\":"));
  EXPECT_FALSE(lazy_ok("[[["));
}

// Invalid literals: prefix-length check fails
TEST(LazyErrors, InvalidLiterals) {
  EXPECT_FALSE(lazy_ok("tru"));
  EXPECT_FALSE(lazy_ok("truth"));
  EXPECT_FALSE(lazy_ok("fal"));
  EXPECT_FALSE(lazy_ok("falsy"));
  EXPECT_FALSE(lazy_ok("nul"));
  EXPECT_FALSE(lazy_ok("nulls"));
}

// Empty input: skip_to_action returns 0 → parse() returns false
TEST(LazyErrors, EmptyInput) {
  EXPECT_FALSE(lazy_ok(""));
  EXPECT_FALSE(lazy_ok("   "));
}

// rtsm: unrecognized value-start chars hit default: return false
TEST(RtsmErrors, UnrecognizedValueChars) {
  EXPECT_FALSE(rtsm_ok("[!]"));
  EXPECT_FALSE(rtsm_ok("[?]"));
  EXPECT_FALSE(rtsm_ok("[&]"));
}

// rtsm: unbalanced depth (depth != 0 at end of input)
// NOTE: short inputs like "[" or "[[]" can overflow GhostTape (init(len/2+1)).
// Use inputs long enough to fit in the pre-allocated tape.
TEST(RtsmErrors, UnbalancedDepth) {
  // len=4 → tape cap=3; pushes: ArrayStart+Num+Num=3; depth=1 at end
  EXPECT_FALSE(rtsm_ok("[1,2"));
  // len=6 → tape cap=4; pushes: ObjectStart+StringRaw+Num=3; depth=1 at end
  EXPECT_FALSE(rtsm_ok("{\"a\":1"));
  // len=14 → tape cap=8; pushes: ObjectStart+StringRaw+StringRaw=3; depth=1
  EXPECT_FALSE(rtsm_ok("{\"key\":\"value\""));
}
