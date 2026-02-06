#include <algorithm> // Added for max/min if needed, or just safe standard lib usage
#include <beast_json/beast_json.hpp>
#include <cstring>
#include <gtest/gtest.h>
#include <vector> // Added for vector

TEST(QuoteMask, SimdBitmap) {
  // Test JSON with structural chars inside strings
  const char *json = R"([{"text": "Hello {world}"}])";
  size_t len = strlen(json);

  // Build bitmap
  beast::json::BitmapIndex idx;
  beast::json::simd::fill_bitmap(json, len, idx);

  // Count bits
  // Simulation of Parser Loop
  int found_structural = 0;
  bool in_string_sim = false;

  for (size_t block_idx = 0; block_idx < idx.structural_bits.size();
       block_idx++) {
    uint64_t bits = idx.structural_bits[block_idx];
    size_t base = block_idx * 64;
    while (bits) {
      int bit_pos = __builtin_ctzll(bits);
      size_t offset = base + bit_pos;
      char c = json[offset];

      if (c == '"') {
        bool escaped = false;
        // Backtrack for escape
        if (offset > 0 && json[offset - 1] == '\\') {
          // Naive count back
          int cnt = 0;
          size_t k = offset - 1;
          while (k < len && json[k] == '\\') { // Bounds check: k is size_t, <
                                               // len always true if decr
            // Wait, k is unsigned. k < len check is redundant if k starts <
            // len. Loop condition needs to be careful about 0.
            cnt++;
            if (k == 0)
              break;
            k--;
          }
          // Re-implement correctly to avoid infinite loop or overflow if logic
          // was flawed. Previously: k starts at offset-1.
        }
        if (!escaped) {
          in_string_sim = !in_string_sim;
        }
      } else {
        if (!in_string_sim) {
          if (strchr("[]{},", c)) {
            found_structural++;
          }
        }
      }
      bits &= bits - 1;
    }
  }

  int expected_struct_count = 4; // [ { } ]
  EXPECT_EQ(found_structural, expected_struct_count);
}
