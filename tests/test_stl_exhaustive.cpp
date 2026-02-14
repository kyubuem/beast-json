#include <beast_json/beast_json.hpp>
#include <deque>
#include <forward_list>
#include <gtest/gtest.h>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace beast::json;

// Helper to check roundtrip
template <typename T> void CheckRoundTrip(const T &input) {
  std::string json = json_of(input);
  // std::cout << "JSON: " << json << std::endl;

  T output;
  Error err = parse_into(output, json);
  EXPECT_EQ(err, Error::Ok);
  if constexpr (std::is_same_v<T, std::forward_list<typename T::value_type>>) {
    // forward_list doesn't have operator==
    EXPECT_TRUE(
        std::equal(input.begin(), input.end(), output.begin(), output.end()));
  } else {
    EXPECT_EQ(input, output);
  }
}

TEST(StlExhaustive, SequenceContainers) {
  CheckRoundTrip(std::vector<int>{1, 2, 3});
  CheckRoundTrip(std::deque<int>{1, 2, 3});
  CheckRoundTrip(std::list<int>{1, 2, 3});
  CheckRoundTrip(std::forward_list<int>{1, 2, 3});
}

TEST(StlExhaustive, AssociativeContainers) {
  CheckRoundTrip(std::set<int>{1, 2, 3});
  CheckRoundTrip(std::multiset<int>{1, 2, 2, 3});

  CheckRoundTrip(std::map<std::string, int>{{"a", 1}, {"b", 2}});
  CheckRoundTrip(std::multimap<std::string, int>{{"a", 1}, {"a", 2}});
}

TEST(StlExhaustive, UnorderedContainers) {
  CheckRoundTrip(std::unordered_set<int>{1, 2, 3});
  CheckRoundTrip(std::unordered_multiset<int>{1, 2, 2, 3});

  CheckRoundTrip(std::unordered_map<std::string, int>{{"a", 1}, {"b", 2}});
  CheckRoundTrip(std::unordered_multimap<std::string, int>{{"a", 1}, {"a", 2}});
}
