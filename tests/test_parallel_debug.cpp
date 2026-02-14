#include <beast_json/beast_json.hpp>
#include <fstream>
#include <gtest/gtest.h>
#include <vector>

TEST(ParallelDebug, AnomalyDetection) {
  // Load twitter.json
  std::ifstream f("twitter.json");
  ASSERT_TRUE(f.is_open());
  std::string json_str((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
  f.close();

  // Create large JSON
  std::string large_json;
  large_json.reserve(json_str.size() * 200 + 202);
  large_json += "[";
  for (int i = 0; i < 200; ++i) {
    large_json += json_str;
    if (i < 199)
      large_json += ",";
  }
  large_json += "]";

  // Build bitmap
  beast::json::BitmapIndex idx;
  beast::json::simd::fill_bitmap(large_json.c_str(), large_json.size(), idx);

  // Manually scan
  int depth = 0;
  size_t block_idx = 0;
  uint64_t bits = idx.structural_bits[0];
  size_t base_offset = 0;
  bool in_string = false;

  while (block_idx < idx.structural_bits.size()) {
    while (bits != 0) {
      int bit_pos = __builtin_ctzll(bits);
      size_t offset = base_offset + bit_pos;

      if (offset >= large_json.size())
        break;

      char c = large_json[offset];

      // Handle String State
      if (c == '"') {
        // Check for escape
        bool escaped = false;
        if (offset > 0 && large_json[offset - 1] == '\\') {
          size_t backslash_count = 0;
          size_t j = offset - 1;
          while (j < large_json.size() && large_json[j] == '\\') {
            backslash_count++;
            if (j == 0)
              break;
            j--;
          }
          if (backslash_count % 2 == 1)
            escaped = true;
        }
        if (!escaped) {
          in_string = !in_string;
          bits &= bits - 1;
          continue;
        }
      }

      if (in_string) {
        bits &= bits - 1;
        continue;
      }

      // Update depth logic
      if (c == '[' || c == '{')
        depth++;
      else if (c == ']' || c == '}')
        depth--;

      ASSERT_GE(depth, 0) << "Depth went negative at offset " << offset;

      bits &= bits - 1;
    }
    block_idx++;
    if (block_idx >= idx.structural_bits.size())
      break;
    bits = idx.structural_bits[block_idx];
    base_offset = block_idx * 64;
  }
  ASSERT_EQ(depth, 0);
}
