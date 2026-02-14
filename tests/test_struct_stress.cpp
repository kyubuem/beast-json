#include <beast_json/beast_json.hpp>
#include <deque>
#include <gtest/gtest.h>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct SubStruct {
  int x;
  std::string y;

  bool operator==(const SubStruct &other) const {
    return x == other.x && y == other.y;
  }
};
BEAST_DEFINE_JSON(SubStruct, x, y)

struct GodStructPrims {
  int8_t i8;
  int16_t i16;
  int32_t i32;
  int64_t i64;
  uint64_t u64;
  float f32;
  double f64;
  bool b;
  std::string str;

  bool operator==(const GodStructPrims &other) const {
    return i8 == other.i8 && i16 == other.i16 && i32 == other.i32 &&
           i64 == other.i64 && u64 == other.u64 && f32 == other.f32 &&
           f64 == other.f64 && b == other.b && str == other.str;
  }
};
BEAST_DEFINE_JSON(GodStructPrims, i8, i16, i32, i64, u64, f32, f64, b, str)

struct GodStructContainers {
  std::vector<int> vec;
  std::list<std::string> lst;
  std::deque<double> deq;
  std::set<int> set_i;
  std::unordered_set<std::string> uset_s;

  bool operator==(const GodStructContainers &other) const {
    return vec == other.vec && lst == other.lst && deq == other.deq &&
           set_i == other.set_i && uset_s == other.uset_s;
  }
};
BEAST_DEFINE_JSON(GodStructContainers, vec, lst, deq, set_i, uset_s)

struct GodStructAdv {
  std::map<std::string, int> map_si;
  std::unordered_map<std::string, SubStruct> umap_ss;
  std::optional<SubStruct> opt_s;
  std::unique_ptr<SubStruct> uniq_s;
  std::shared_ptr<SubStruct> shar_s;

  bool operator==(const GodStructAdv &other) const {
    if (!(map_si == other.map_si && opt_s == other.opt_s))
      return false;
    if (umap_ss.size() != other.umap_ss.size())
      return false;
    for (auto const &[key, val] : umap_ss) {
      auto it = other.umap_ss.find(key);
      if (it == other.umap_ss.end() || !(it->second == val))
        return false;
    }
    if (uniq_s && other.uniq_s) {
      if (!(*uniq_s == *other.uniq_s))
        return false;
    } else if (uniq_s || other.uniq_s)
      return false;
    if (shar_s && other.shar_s) {
      if (!(*shar_s == *other.shar_s))
        return false;
    } else if (shar_s || other.shar_s)
      return false;
    return true;
  }
};
BEAST_DEFINE_JSON(GodStructAdv, map_si, umap_ss, opt_s, uniq_s, shar_s)

struct GodStruct {
  GodStructPrims prims;
  GodStructContainers conts;
  GodStructAdv adv;

  bool operator==(const GodStruct &other) const {
    return prims == other.prims && conts == other.conts && adv == other.adv;
  }
};
BEAST_DEFINE_JSON(GodStruct, prims, conts, adv)

TEST(StructStress, GodStructRoundTrip) {
  GodStruct g;
  g.prims.i8 = -8;
  g.prims.i16 = -1600;
  g.prims.i32 = -320000;
  g.prims.i64 = -6400000000LL;
  g.prims.u64 = 123456789012345ULL;
  g.prims.f32 = 3.14f;
  g.prims.f64 = 2.718281828;
  g.prims.b = true;
  g.prims.str = "Hello \"Beast\" \n JSON!";

  g.conts.vec = {1, 2, 3};
  g.conts.lst = {"a", "b", "c"};
  g.conts.deq = {1.1, 2.2};
  g.conts.set_i = {10, 20, 30};
  g.conts.uset_s = {"alpha", "beta"};

  g.adv.map_si = {{"one", 1}, {"two", 2}};
  g.adv.umap_ss = {{"first", {1, "sub1"}}, {"second", {2, "sub2"}}};
  g.adv.opt_s = SubStruct{100, "optional"};
  g.adv.uniq_s = std::make_unique<SubStruct>(SubStruct{200, "unique"});
  g.adv.shar_s = std::make_shared<SubStruct>(SubStruct{300, "shared"});

  // Round trip
  std::string json = beast::json::json_of(g);
  std::cout << "Serialized JSON: " << json << std::endl;

  GodStruct g2;
  beast::json::Error err = beast::json::parse_into(g2, json);
  ASSERT_EQ(err, beast::json::Error::Ok)
      << "Parse failed: " << beast::json::error_message(err);

  // EXPECT_TRUE(g == g2);
  EXPECT_EQ(g.prims.i8, g2.prims.i8);
  EXPECT_EQ(g.prims.i16, g2.prims.i16);
  EXPECT_EQ(g.prims.i32, g2.prims.i32);
  EXPECT_EQ(g.prims.i64, g2.prims.i64);
  EXPECT_EQ(g.prims.u64, g2.prims.u64);
  EXPECT_NEAR(g.prims.f32, g2.prims.f32, 1e-5);
  EXPECT_NEAR(g.prims.f64, g2.prims.f64, 1e-9);
  EXPECT_EQ(g.prims.b, g2.prims.b);
  EXPECT_EQ(g.prims.str, g2.prims.str);

  EXPECT_EQ(g.conts.vec, g2.conts.vec);
  EXPECT_EQ(g.conts.lst, g2.conts.lst);
  EXPECT_EQ(g.conts.deq, g2.conts.deq);
  EXPECT_EQ(g.conts.set_i, g2.conts.set_i);
  EXPECT_EQ(g.conts.uset_s, g2.conts.uset_s);

  EXPECT_EQ(g.adv.map_si, g2.adv.map_si);
  ASSERT_EQ(g.adv.umap_ss.size(), g2.adv.umap_ss.size());
  for (auto const &[k, v] : g.adv.umap_ss) {
    ASSERT_TRUE(g2.adv.umap_ss.count(k));
    EXPECT_EQ(v.x, g2.adv.umap_ss.at(k).x);
    EXPECT_EQ(v.y, g2.adv.umap_ss.at(k).y);
  }

  EXPECT_TRUE(g2.adv.opt_s.has_value());
  EXPECT_EQ(g.adv.opt_s->x, g2.adv.opt_s->x);
  EXPECT_EQ(g.adv.opt_s->y, g2.adv.opt_s->y);

  ASSERT_TRUE(g2.adv.uniq_s);
  EXPECT_EQ(g.adv.uniq_s->x, g2.adv.uniq_s->x);
  EXPECT_EQ(g.adv.uniq_s->y, g2.adv.uniq_s->y);

  ASSERT_TRUE(g2.adv.shar_s);
  EXPECT_EQ(g.adv.shar_s->x, g2.adv.shar_s->x);
  EXPECT_EQ(g.adv.shar_s->y, g2.adv.shar_s->y);

  // Test null smart pointers/optional
  g.adv.opt_s = std::nullopt;
  g.adv.uniq_s.reset();
  g.adv.shar_s.reset();

  json = beast::json::json_of(g);
  GodStruct g3;
  err = beast::json::parse_into(g3, json);
  ASSERT_EQ(err, beast::json::Error::Ok);
  EXPECT_FALSE(g3.adv.opt_s.has_value());
  EXPECT_FALSE(g3.adv.uniq_s);
  EXPECT_FALSE(g3.adv.shar_s);
}
