#include <beast_json/beast_json.hpp>
#include <fstream>
#include <gtest/gtest.h>

TEST(Bitmap, SimpleDebugTrace) {
  std::ifstream f("test_simple.json");
  ASSERT_TRUE(f.is_open()) << "Failed to open test_simple.json";
  std::string json((std::istreambuf_iterator<char>(f)),
                   std::istreambuf_iterator<char>());
  f.close();

  beast::json::BitmapIndex idx;
  beast::json::simd::fill_bitmap(json.c_str(), json.size(), idx);

  // Iterate and track depth
  int depth = 0;
  size_t block_idx = 0;
  uint64_t bits = idx.structural_bits[0];
  size_t base_offset = 0;
  size_t scanned = 0;

  while (block_idx < idx.structural_bits.size() && scanned < 30) {
    while (bits != 0 && scanned < 30) {
      int bit_pos = __builtin_ctzll(bits);
      size_t offset = base_offset + bit_pos;
      if (offset >= json.size())
        break;
      char c = json[offset];

      if (c == '[' || c == '{')
        depth++;
      else if (c == ']' || c == '}')
        depth--;

      scanned++;
      bits &= bits - 1;

      ASSERT_GE(depth, 0) << "Depth went negative at offset " << offset;
    }
    block_idx++;
    if (block_idx < idx.structural_bits.size()) {
      bits = idx.structural_bits[block_idx];
      base_offset = block_idx * 64;
    }
  }
}
