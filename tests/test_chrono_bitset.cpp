#include <beast_json/beast_json.hpp>
#include <bitset>
#include <chrono>
#include <gtest/gtest.h>

using namespace beast::json;

// Helper to check roundtrip
template <typename T> void CheckRoundTrip(const T &input) {
  std::string json = json_of(input);
  // std::cout << "JSON: " << json << std::endl;

  T output;
  Error err = parse_into(output, json);
  EXPECT_EQ(err, Error::Ok);
  EXPECT_EQ(input, output);
}

TEST(ChronoAndBitset, Duration) {
  using namespace std::chrono_literals;
  auto d1 = 123ms;
  CheckRoundTrip(d1);

  auto d2 = 456h;
  CheckRoundTrip(d2);
}

TEST(ChronoAndBitset, TimePoint) {
  auto now = std::chrono::system_clock::now();
  // Precision might be lost if JSON integer size isn't enough?
  // beast_json supports uint64_t, which is enough for nanoseconds since epoch
  // usually. However, system_clock might use different rep. Let's cast to a
  // known one.

  // Cast to seconds for safety if rep is weird, but let's try raw first.
  CheckRoundTrip(now);
}

TEST(ChronoAndBitset, Bitset) {
  std::bitset<8> b8(0b10101010);
  CheckRoundTrip(b8);

  std::bitset<64> b64(0xFFFFFFFFFFFFFFFF);
  CheckRoundTrip(b64);

  std::bitset<128> b128; // All zero
  CheckRoundTrip(b128);
}
