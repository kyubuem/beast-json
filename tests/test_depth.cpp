#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(Depth, CommaDetection) {
  // Simple test: [{1},{2},{3}]
  std::string json = "[{\"a\":1},{\"b\":2},{\"c\":3}]";

  int depth = 0;
  std::vector<size_t> depth1_commas;

  for (size_t i = 0; i < json.size(); i++) {
    char c = json[i];

    // Skip non-structural chars
    if (c != '[' && c != ']' && c != '{' && c != '}' && c != ',')
      continue;

    // Check comma BEFORE updating depth
    if (depth == 1 && c == ',') {
      depth1_commas.push_back(i);
    }

    // Update depth
    if (c == '[' || c == '{')
      depth++;
    else if (c == ']' || c == '}')
      depth--;

    ASSERT_GE(depth, 0) << "Depth went negative at index " << i;
  }

  EXPECT_EQ(depth1_commas.size(), 2);
  // Expected: 2 (between {1},{2} and {2},{3})
}
