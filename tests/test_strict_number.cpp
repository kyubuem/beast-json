#include <beast_json/beast_json.hpp>
#include <gtest/gtest.h>
#include <string_view>

using namespace beast::json;

// NOTE: The rtsm::Parser uses a lenient digit scanner: any sequence of
// digits, '.', 'e', 'E', '-', '+' is accepted as a number. Leading zeros,
// trailing dots, incomplete exponents are all accepted.
// Only chars not in the switch statement ('+', '.') fail as value-starts.

static bool rtsm_ok(std::string_view j) {
  try {
    beast::json::parse(j);
    return true;
  } catch (const std::runtime_error &) {
    return false;
  }
}

// Valid RFC 8259 numbers: always accepted
TEST(StrictNumber, ValidNumbersAccepted) {
  EXPECT_TRUE(rtsm_ok("0"));
  EXPECT_TRUE(rtsm_ok("123"));
  EXPECT_TRUE(rtsm_ok("-5"));
  EXPECT_TRUE(rtsm_ok("3.14"));
  EXPECT_TRUE(rtsm_ok("1.5e2"));
  EXPECT_TRUE(rtsm_ok("-0.5"));
  EXPECT_TRUE(rtsm_ok("1e10"));
  EXPECT_TRUE(rtsm_ok("-1.5e-3"));
}

// Lenient scanner: accepts non-RFC number formats
// (rtsm scan loop includes '.', 'e', 'E', '-', '+' in digit chars)
TEST(StrictNumber, LenientScannerBehavior) {
  EXPECT_TRUE(rtsm_ok("01"));    // leading zero: accepted
  EXPECT_TRUE(rtsm_ok("007"));   // leading zeros: accepted
  EXPECT_TRUE(rtsm_ok("1."));    // trailing dot: accepted (flt=true, no digit check)
  EXPECT_TRUE(rtsm_ok("1e"));    // incomplete exponent: scan includes 'e'
}

// Invalid value-start chars: not in rtsm switch -> default: return false
TEST(StrictNumber, InvalidStartCharsRejected) {
  EXPECT_FALSE(rtsm_ok("+123")); // '+' not in number case
  EXPECT_FALSE(rtsm_ok(".5"));   // '.' not in number case
}

// Valid negative zero
TEST(StrictNumber, NegativeZeroAccepted) {
  EXPECT_TRUE(rtsm_ok("-0"));
}
